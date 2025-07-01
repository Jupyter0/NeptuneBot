using System.Text.Json;
using System.Diagnostics;
using System.Text;

class Program
{
    static string botUsername = "";
    static readonly HttpClient client = new HttpClient();
    

    static async Task Main(string[] args)
    {
        if (args.Length != 2) Environment.Exit(160);
        
        string envFile = args[0].Length > 0 ? args[0] : ".env";
        botUsername = args[1].Length > 0 ? args[1] : "Neptune-Bot";
        DotNetEnv.Env.Load(envFile);

        string token = Environment.GetEnvironmentVariable("LICHESS_TOKEN")
        ?? throw new InvalidOperationException("LICHESS_TOKEN not set");

        client.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);


        var stream = await client.GetStreamAsync("https://lichess.org/api/stream/event");
        using var reader = new StreamReader(stream);

        while (!reader.EndOfStream)
        {
            var line = await reader.ReadLineAsync();
            if (string.IsNullOrWhiteSpace(line)) continue;

            using var doc = JsonDocument.Parse(line);
            if (!doc.RootElement.TryGetProperty("type", out var typeElement)) continue;

            var type = typeElement.GetString();

            if (type == "gameStart") await HandleGameStart(doc);
            else if (type == "challenge") await HandleChallenge(doc);
        }
    }

    static async Task HandleGameStart(JsonDocument doc)
    {
        if (!doc.RootElement.TryGetProperty("game", out var gameElement)) return;
        if (!gameElement.TryGetProperty("id", out var idElement)) return;

        var gameId = idElement.GetString();
        if (string.IsNullOrEmpty(gameId)) return;

        await HandleGame(gameId);
    }

    static async Task HandleChallenge(JsonDocument doc)
    {
        if (!doc.RootElement.TryGetProperty("challenge", out var challengeElement)) return;
        if (!challengeElement.TryGetProperty("id", out var challengeIdElement)) return;

        var challengeId = challengeIdElement.GetString();
        if (string.IsNullOrEmpty(challengeId)) return;

        Console.WriteLine($"[CHALLENGE] Received challenge: {challengeId}");

        var urlAccept = $"https://lichess.org/api/challenge/{challengeId}/accept";
        var res = await client.PostAsync(urlAccept, null);
        if (res.IsSuccessStatusCode)
        {
            Console.WriteLine($"[CHALLENGE] Accepted challenge {challengeId}");
        }
        else
            Console.WriteLine($"[CHALLENGE] Failed to accept challenge: {await res.Content.ReadAsStringAsync()}");
    }

    static async Task HandleGame(string gameId)
    {
        try
        {
            var url = $"https://lichess.org/api/bot/game/stream/{gameId}";
            var stream = await client.GetStreamAsync(url);
            using var reader = new StreamReader(stream);

            await HandleGameStream(reader, gameId);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[ERROR] {ex.Message}\n{ex.StackTrace}");
        }
    }

    static async Task HandleGameStream(StreamReader reader, string gameId)
    {
        string? color = null;
        string? fen = null;
        List<string> moves = new();
        int lastMoveCount = -1;

        using var engine = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "./engine/bin/Neptune",
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            },
            EnableRaisingEvents = true
        };

        engine.Start();

        while (!reader.EndOfStream)
        {
            var line = await reader.ReadLineAsync();
            if (string.IsNullOrWhiteSpace(line)) continue;

            using var doc = JsonDocument.Parse(line);
            var type = doc.RootElement.TryGetProperty("type", out var t) ? t.GetString() : null;

            if (type == "gameFull")
            {
                HandleGameFull(doc, ref color, ref fen, ref moves, engine);
            }
            else if (type == "gameState")
            {
                HandleGameState(doc, ref moves);

                if (doc.RootElement.TryGetProperty("status", out var statusElement))
                {
                    var status = statusElement.GetString();
                    if (status != "started")
                    {
                        //Console.WriteLine($"[INFO] Game ended or aborted (status: {status})");
                        engine.Kill();
                        break;
                    }
                }
            }

            if (!string.IsNullOrEmpty(color) &&
                !string.IsNullOrEmpty(fen) &&
                IsMyTurn(color, moves.Count) &&
                (moves.Count != lastMoveCount))
            {
                var move = await GetBestMoveFromEngine(fen, moves, engine);
                await SendMove(gameId, move.Trim());
                lastMoveCount = moves.Count;
            }
        }
    }

    static void HandleGameState(JsonDocument doc, ref List<string> moves)
    {
        if (doc.RootElement.TryGetProperty("moves", out var moveStr))
        {
            var moveText = moveStr.GetString();
            if (!string.IsNullOrEmpty(moveText))
                moves = new(moveText.Split(' ', StringSplitOptions.RemoveEmptyEntries));
        }
    }

    static void HandleGameFull(JsonDocument doc, ref string? color, ref string? fen, ref List<string> moves, Process engine)
    {
        if (doc.RootElement.TryGetProperty("white", out var white) &&
            white.TryGetProperty("id", out var whiteIdElement))
        {
            var whiteId = whiteIdElement.GetString();
            if (!string.IsNullOrEmpty(whiteId))
            {
                color = whiteId.Equals(botUsername, StringComparison.OrdinalIgnoreCase) ? "white" : "black";
            }
        }

        if (doc.RootElement.TryGetProperty("initialFen", out var fenElement))
        {
            fen = fenElement.GetString();
            if (fen == "startpos")
                fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

            engine.StandardInput.WriteLineAsync($"initial {fen}");
            engine.StandardInput.FlushAsync();
        }

        if (doc.RootElement.TryGetProperty("state", out var state) &&
            state.TryGetProperty("moves", out var moveStr))
        {
            var moveText = moveStr.GetString();
            if (!string.IsNullOrEmpty(moveText))
                moves = new(moveText.Split(' ', StringSplitOptions.RemoveEmptyEntries));
        }
    }

    static string GetTurnFromFEN(string fen, List<string> moves)
    {
        var parts = fen.Split(' ');
        return parts.Length > 1 ? (parts[1] == "w" ? "white" : "black") : "white";
    }

    static bool IsMyTurn(string color, int moveCount)
    {
        return (color == "white" && moveCount % 2 == 0) ||
               (color == "black" && moveCount % 2 == 1);
    }

    static async Task<string> GetBestMoveFromEngine(string fen, List<string> moves, Process engine)
    {
        string latestMove = moves.Count > 0 ? moves[moves.Count - 1] : "start";

        await engine.StandardInput.WriteLineAsync($"move {latestMove}");
        await engine.StandardInput.WriteLineAsync("go wtime 30000 btime 30000");
        await engine.StandardInput.FlushAsync();

        string? line;
        while ((line = await engine.StandardOutput.ReadLineAsync()) != null)
        {
            if (line.StartsWith("[TIME]") || line.StartsWith("[LOG]")) Console.WriteLine(line);
            if (line.StartsWith("bestmove"))
            {
                return line.Split(' ')[1];
            }
        }

        return "0000";
    }

    static async Task SendMove(string gameId, string move)
    {
        move = move.Trim();
        gameId = gameId.Trim();
        move = new string(move.Where(c => c >= 32 && c <= 126).ToArray());
        var request = new HttpRequestMessage
        {
            Method = HttpMethod.Post,
            RequestUri = new Uri($"https://lichess.org/api/bot/game/{gameId.Trim()}/move/{move}"),
            Content = null
        };
        var res = await client.SendAsync(request);

        if (res.IsSuccessStatusCode)
        {
            //Console.WriteLine($"[MOVE] {move} sent successfully");
        }
        else
        {
            var errorBody = await res.Content.ReadAsStringAsync();
            Console.WriteLine($"[ERROR] Failed to send move: {(int)res.StatusCode} {res.ReasonPhrase}");
            Console.WriteLine($"[ERROR] Response content: {errorBody}");
        }
    }
}