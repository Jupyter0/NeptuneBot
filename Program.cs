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

        while (!reader.EndOfStream)
        {
            var line = await reader.ReadLineAsync();
            if (string.IsNullOrWhiteSpace(line)) continue;

            using var doc = JsonDocument.Parse(line);
            var type = doc.RootElement.TryGetProperty("type", out var t) ? t.GetString() : null;

            if (type == "gameFull")
            {
                HandleGameFull(doc, ref color, ref fen, ref moves);
            }
            else if (type == "gameState")
            {
                HandleGameState(doc, ref moves);

                if (doc.RootElement.TryGetProperty("status", out var statusElement))
                {
                    var status = statusElement.GetString();
                    if (status != "started")
                    {
                        Console.WriteLine($"[INFO] Game ended or aborted (status: {status})");
                        break;
                    }
                }
            }

            if (!string.IsNullOrEmpty(color) &&
                !string.IsNullOrEmpty(fen) &&
                IsMyTurn(color, moves.Count) &&
                moves.Count != lastMoveCount)
            {
                var move = await GetBestMoveFromEngine(fen, moves);
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

    static void HandleGameFull(JsonDocument doc, ref string? color, ref string? fen, ref List<string> moves)
    {
        if (doc.RootElement.TryGetProperty("white", out var white) &&
            white.TryGetProperty("id", out var whiteIdElement))
        {
            var whiteId = whiteIdElement.GetString();
            if (!string.IsNullOrEmpty(whiteId))
                color = whiteId.Equals(botUsername, StringComparison.OrdinalIgnoreCase) ? "white" : "black";
        }

        if (doc.RootElement.TryGetProperty("initialFen", out var fenElement))
        {
            fen = fenElement.GetString();
            if (fen == "startpos")
                fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
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
        var pos = new ChessPosition();

        foreach (var move in moves)
        {
            pos.ApplyMove(move);
        }
        string uFen = pos.GetFEN();
        await engine.StandardInput.WriteLineAsync($"position fen {uFen} moves {moveStr}");
        await engine.StandardInput.WriteLineAsync("go wtime 30000 btime 30000");
        await engine.StandardInput.FlushAsync();

        string? line;
        while ((line = await engine.StandardOutput.ReadLineAsync()) != null)
        {
            //Console.WriteLine($"[Neptune]: {line}");
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
            Console.WriteLine($"[MOVE] {move} sent successfully");
        }
        else
        {
            var errorBody = await res.Content.ReadAsStringAsync();
            Console.WriteLine($"[ERROR] Failed to send move: {(int)res.StatusCode} {res.ReasonPhrase}");
            Console.WriteLine($"[ERROR] Response content: {errorBody}");
        }
    }
}

public class ChessPosition
{
    private char[,] board = new char[8, 8];
    private bool whiteToMove = true;
    private bool whiteCanCastleK = true, whiteCanCastleQ = true;
    private bool blackCanCastleK = true, blackCanCastleQ = true;
    private int? enPassantFile = null; // 0..7 or null if none
    private int halfmoveClock = 0;
    private int fullmoveNumber = 1;

    public ChessPosition()
    {
        SetStartPosition();
    }

    public void SetStartPosition()
    {
        string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
        var ranks = startFEN.Split('/');
        for (int r = 0; r < 8; r++)
        {
            int file = 0;
            foreach (char c in ranks[r])
            {
                if (char.IsDigit(c))
                {
                    int empty = c - '0';
                    for (int i = 0; i < empty; i++) board[r, file++] = ' ';
                }
                else
                {
                    board[r, file++] = c;
                }
            }
        }

        whiteToMove = true;
        whiteCanCastleK = whiteCanCastleQ = true;
        blackCanCastleK = blackCanCastleQ = true;
        enPassantFile = null;
        halfmoveClock = 0;
        fullmoveNumber = 1;
    }

    private static int RankFromChar(char c) => 8 - (c - '0');
    private static int FileFromChar(char c) => c - 'a';

    public void ApplyMove(string move)
    {
        // move example: e2e4 or e7e8q
        int fromFile = FileFromChar(move[0]);
        int fromRank = 8 - (move[1] - '0');
        int toFile = FileFromChar(move[2]);
        int toRank = 8 - (move[3] - '0');

        char movingPiece = board[fromRank, fromFile];
        char capturedPiece = board[toRank, toFile];

        // Reset en passant
        enPassantFile = null;

        // Update halfmove clock
        if (char.ToLower(movingPiece) == 'p' || capturedPiece != ' ')
            halfmoveClock = 0;
        else
            halfmoveClock++;

        // Update castling rights if king or rook moves or rook captured
        UpdateCastlingRights(movingPiece, fromRank, fromFile, toRank, toFile, capturedPiece);

        // Handle castling
        if ((movingPiece == 'K' || movingPiece == 'k') && Math.Abs(toFile - fromFile) == 2)
        {
            if (toFile == 6) // kingside
            {
                board[toRank, 5] = board[toRank, 7];
                board[toRank, 7] = ' ';
            }
            else if (toFile == 2) // queenside
            {
                board[toRank, 3] = board[toRank, 0];
                board[toRank, 0] = ' ';
            }
        }

        // En passant capture
        if (char.ToLower(movingPiece) == 'p' && toFile != fromFile && capturedPiece == ' ')
        {
            if (whiteToMove)
                board[toRank + 1, toFile] = ' ';
            else
                board[toRank - 1, toFile] = ' ';
        }

        // Move piece
        board[toRank, toFile] = movingPiece;
        board[fromRank, fromFile] = ' ';

        // Promotion
        if (move.Length == 5)
        {
            char promo = move[4];
            board[toRank, toFile] = whiteToMove ? char.ToUpper(promo) : char.ToLower(promo);
        }

        // En passant target square (only if pawn double step)
        if (char.ToLower(movingPiece) == 'p' && Math.Abs(toRank - fromRank) == 2)
            enPassantFile = fromFile;

        // Update side and fullmove number
        whiteToMove = !whiteToMove;
        if (whiteToMove)
            fullmoveNumber++;
    }

    private void UpdateCastlingRights(char movingPiece, int fromRank, int fromFile, int toRank, int toFile, char capturedPiece)
    {
        // King moves lose all castling rights of that side
        if (movingPiece == 'K') whiteCanCastleK = whiteCanCastleQ = false;
        if (movingPiece == 'k') blackCanCastleK = blackCanCastleQ = false;

        // Rook moves lose respective castling rights
        if (movingPiece == 'R')
        {
            if (fromRank == 7 && fromFile == 0) whiteCanCastleQ = false;
            else if (fromRank == 7 && fromFile == 7) whiteCanCastleK = false;
        }
        else if (movingPiece == 'r')
        {
            if (fromRank == 0 && fromFile == 0) blackCanCastleQ = false;
            else if (fromRank == 0 && fromFile == 7) blackCanCastleK = false;
        }

        // Rook captures lose castling rights
        if (capturedPiece == 'R')
        {
            if (toRank == 7 && toFile == 0) whiteCanCastleQ = false;
            else if (toRank == 7 && toFile == 7) whiteCanCastleK = false;
        }
        else if (capturedPiece == 'r')
        {
            if (toRank == 0 && toFile == 0) blackCanCastleQ = false;
            else if (toRank == 0 && toFile == 7) blackCanCastleK = false;
        }
    }

    public string GetFEN()
    {
        StringBuilder sb = new StringBuilder();

        for (int r = 0; r < 8; r++)
        {
            int emptyCount = 0;
            for (int f = 0; f < 8; f++)
            {
                char c = board[r, f];
                if (c == ' ') emptyCount++;
                else
                {
                    if (emptyCount > 0)
                    {
                        sb.Append(emptyCount);
                        emptyCount = 0;
                    }
                    sb.Append(c);
                }
            }
            if (emptyCount > 0) sb.Append(emptyCount);
            if (r < 7) sb.Append('/');
        }

        sb.Append(' ');
        sb.Append(whiteToMove ? 'w' : 'b');
        sb.Append(' ');

        string castlingRights = "";
        if (whiteCanCastleK) castlingRights += 'K';
        if (whiteCanCastleQ) castlingRights += 'Q';
        if (blackCanCastleK) castlingRights += 'k';
        if (blackCanCastleQ) castlingRights += 'q';
        sb.Append(castlingRights.Length > 0 ? castlingRights : "-");
        sb.Append(' ');

        if (enPassantFile.HasValue)
            sb.Append((char)('a' + enPassantFile.Value)).Append(whiteToMove ? '6' : '3');
        else
            sb.Append('-');

        sb.Append(' ').Append(halfmoveClock);
        sb.Append(' ').Append(fullmoveNumber);

        return sb.ToString();
    }
}