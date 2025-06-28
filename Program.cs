using System;
using System.IO;
using System.Net.Http;
using System.Text.Json;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Collections.Generic;

class Program
{
    static readonly string token = "lip_xKibWq61cz9cwlt321Ue";
    static readonly string botUsername = "Neptune-Bot";
    static readonly HttpClient client = new HttpClient();

    static async Task Main()
    {
        client.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        var stream = await client.GetStreamAsync("https://lichess.org/api/stream/event");
        using var reader = new StreamReader(stream);

        Console.WriteLine("[INFO] Listening for events...");

        while (!reader.EndOfStream)
        {
            var line = await reader.ReadLineAsync();
            if (string.IsNullOrWhiteSpace(line)) continue;

            using var doc = JsonDocument.Parse(line);
            if (!doc.RootElement.TryGetProperty("type", out var typeElement)) continue;

            var type = typeElement.GetString();
            Console.WriteLine($"[DEBUG] Event type: {type}");

            if (type == "gameStart") HandleGameStart(doc);
            else if (type == "challenge") HandleChallenge(doc);
        }
    }
    static async void HandleGameStart(JsonDocument doc)
    {
        bool gameExists = doc.RootElement.TryGetProperty("game", out var gameElement);
        bool idExists = gameElement.TryGetProperty("id", out var idElement);

        if (!gameExists && !idExists) return;

        var gameId = idElement.GetString();

        if (string.IsNullOrEmpty(gameId)) return;

        Console.WriteLine($"[GAME] Started: {gameId}");
        await Task.Run(() => HandleGame(gameId));
    }
    static async void HandleChallenge(JsonDocument doc)
    {
        bool challengeExists = doc.RootElement.TryGetProperty("challenge", out var challengeElement);
        bool idExists = challengeElement.TryGetProperty("id", out var challengeIdElement);

        if (!challengeExists && !idExists) return;
        var challengeId = challengeIdElement.GetString();
        if (string.IsNullOrEmpty(challengeId)) return;
        Console.WriteLine($"[CHALLENGE] Received challenge: {challengeId}");
        var urlAccept = $"https://lichess.org/api/challenge/{challengeId}/accept";
        var res = await client.PostAsync(urlAccept, null);
        if (res.IsSuccessStatusCode)
            Console.WriteLine($"[CHALLENGE] Accepted challenge {challengeId}");
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

            HandleGameStream(reader, gameId);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[ERROR] {ex.Message}");
        }
    }

    static async void HandleGameStream(StreamReader reader, string gameId)
    {
        string? color = null;
        string? fen = null;
        List<string> moves = new();
        while (!reader.EndOfStream)
        {
            var line = await reader.ReadLineAsync();
            if (string.IsNullOrWhiteSpace(line)) continue;

            var json = line;
            using var doc = JsonDocument.Parse(json);
            var type = doc.RootElement.TryGetProperty("type", out var t) ? t.GetString() : null;

            Console.WriteLine($"[DEBUG] Event Type: {type}");

            if (type == "gameFull")
            {
                HandleGameFull(doc, color, fen, moves);
            }
            else if (type == "gameState")
            {
                HandleGameState(doc, moves);
            }

            if (!string.IsNullOrEmpty(color) && !string.IsNullOrEmpty(fen) &&
                color == GetTurnFromFEN(fen, moves) && IsMyTurn(color, moves.Count))
            {
                var move = await GetBestMoveFromEngine(fen, moves);
                await SendMove(gameId, move);
            }
        }
    }

    static void HandleGameState(JsonDocument doc, List<string> moves)
    {
        if (doc.RootElement.TryGetProperty("moves", out var moveStr))
        {
            var moveText = moveStr.GetString();
            if (!string.IsNullOrEmpty(moveText))
                moves = new(moveText.Split(' ', StringSplitOptions.RemoveEmptyEntries));
        }
    }

    static void HandleGameFull(JsonDocument doc, string? color, string? fen, List<string> moves)
    {
        bool whiteExists = doc.RootElement.TryGetProperty("white", out var white);
        bool whiteIdExists = white.TryGetProperty("id", out var whiteIdElement);

        if (!whiteExists && !whiteIdExists) return;

        var whiteId = whiteIdElement.GetString();
        if (!string.IsNullOrEmpty(whiteId))
            color = whiteId.Equals(botUsername, StringComparison.OrdinalIgnoreCase) ? "white" : "black";

        bool fenExists = doc.RootElement.TryGetProperty("initialFen", out var fenElement);
        if (!fenExists) return;
        fen = fenElement.GetString();
        if (fen == "startpos")
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

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

    static async Task<string> GetBestMoveFromEngine(string fen, List<string> moves)
    {
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

        string moveStr = string.Join(" ", moves);
        await engine.StandardInput.WriteLineAsync($"position fen {fen} moves {moveStr}");
        await engine.StandardInput.WriteLineAsync("go wtime 30000 btime 30000");
        await engine.StandardInput.FlushAsync();

        string? line;
        while ((line = await engine.StandardOutput.ReadLineAsync()) != null)
        {
            if (line.StartsWith("bestmove"))
            {
                try { engine.Kill(); } catch { }
                return line.Split(' ')[1];
            }
        }

        try { engine.Kill(); } catch { }
        return "0000";
    }

    static async Task SendMove(string gameId, string move)
    {
        var res = await client.PostAsync($"https://lichess.org/api/bot/game/{gameId}/move/{move}", null);
        if (res.IsSuccessStatusCode)
            Console.WriteLine($"[MOVE] {move} sent for {gameId}");
        else
            Console.WriteLine($"[ERROR] Failed to send move: {await res.Content.ReadAsStringAsync()}");
    }
}
