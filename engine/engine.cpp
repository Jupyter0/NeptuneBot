#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <sstream>
#include <cctype>
#include <iterator>

using namespace std;

constexpr int bishopDirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
constexpr int rookDirs[4][2] = {{0, 1}, {0, -1}, {-1, 0}, {1, 0}};
constexpr int queenDirs[8][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {0, 1}, {0, -1}, {-1, 0}, {1, 0}};

enum Piece {
    EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Color {
    NONE, WHITE, BLACK
};

struct Move {
    int from;
    int to;
    char promotion = 0;

    Move(int From, int To, char Promotion = 0) : from(From), to(To), promotion(Promotion) {}
};

class Board {
public:
    uint64_t whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKing;
    uint64_t blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKing;

    uint64_t whitePieces;
    uint64_t blackPieces;
    uint64_t allPieces;

    bool whiteToMove;

    uint8_t castlingRights; //0b0000KQkq K: WKS, Q: WQS, k: BKS, q: BQS
    uint8_t enPassantSquare; //0b0fdddddd f: flag, d: data

    uint8_t halfmoveClock; //Used for 50-move rule
    uint8_t fullmoveNumber; //Counts the move number

    uint64_t zobristKey = 0ULL; //Zobrist Hash !!Don't use this, this feature is not yet implemented!!

    void setBB(const string& fen) {
        if (fen.length() == 0) return;
        istringstream fss(fen);
        string field;
        vector<string> fields;

        while (fss >> field) {
            fields.push_back(field);
        }

        if (fields.size() < 6) {
            cerr << "Invalid FEN: not enough fields\n";
            return;
        }


        string pieces = fields[0];
        vector<string> ranks;
        size_t start = 0;
        stringstream ss(fields[0]);
        string rank;
        while (getline(ss, rank, '/')) ranks.push_back(rank);

        whitePawns = whiteKnights = whiteBishops = whiteRooks = whiteQueens = whiteKing = 0ULL;
        blackPawns = blackKnights = blackBishops = blackRooks = blackQueens = blackKing = 0ULL;

        for (uint8_t i = 0; i < 8 ; i++) {
            uint8_t file = 0;
            for (uint8_t j = 0; j < ranks[i].length(); j++) {
                char piece = ranks[i][j];
                if (isdigit(piece)) {
                    file += piece - '0';
                    continue;
                }
                uint8_t squareIndex = (7-i)*8 + file; // Board square index: a1 = 0, h8 = 63 (bottom-left to top-right)
                switch (piece) {
                        case 'P': whitePawns     |= 1ULL << squareIndex; break;
                        case 'N': whiteKnights   |= 1ULL << squareIndex; break;
                        case 'B': whiteBishops   |= 1ULL << squareIndex; break;
                        case 'R': whiteRooks     |= 1ULL << squareIndex; break;
                        case 'Q': whiteQueens    |= 1ULL << squareIndex; break;
                        case 'K': whiteKing      |= 1ULL << squareIndex; break;

                        case 'p': blackPawns     |= 1ULL << squareIndex; break;
                        case 'n': blackKnights   |= 1ULL << squareIndex; break;
                        case 'b': blackBishops   |= 1ULL << squareIndex; break;
                        case 'r': blackRooks     |= 1ULL << squareIndex; break;
                        case 'q': blackQueens    |= 1ULL << squareIndex; break;
                        case 'k': blackKing      |= 1ULL << squareIndex; break;
                        default:
                            cerr << "[Warning]: Unknown piece character '" << piece << "'\n";
                            break;
                }
                file++;
            }
        }

        whiteToMove = (fields[1] == "w");

        castlingRights = 0ULL;
        if (fields[2] != "-") {
            for (char c : fields[2]) {
                switch (c) {
                    case 'K': castlingRights |= 1 << 3; break;
                    case 'Q': castlingRights |= 1 << 2; break;
                    case 'k': castlingRights |= 1 << 1; break;
                    case 'q': castlingRights |= 1 << 0; break;
                }
            }
        }

        constexpr uint8_t epFlag = 1 << 6;
        enPassantSquare = 0ULL;
        if (fields[3] != "-") {
            int file = fields[3][0] - 'a';
            int rank = fields[3][1] - '1';
            if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                enPassantSquare = epFlag | (rank * 8 + file);
            } else {
                cerr << "[Warning] Invalid en passant square in FEN: " << fields[3] << endl;
            }
        }

        halfmoveClock = std::stoi(fields[4]);
        fullmoveNumber = std::stoi(fields[5]);

        UpdateOccupancy();
    }

    void make_move(const Move& move) {}
    bool is_king_in_check(bool white) { return false; }


    void UpdateOccupancy() {
        whitePieces = whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
        blackPieces = blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
        allPieces = whitePieces | blackPieces;
    }

    bool hasEnPassant() const {
        return (enPassantSquare & (1 << 6)) != 0;
    }

    uint8_t getEnPassantTarget() const {
        // Returns 0–63 square index if en passant is available; undefined otherwise
        return enPassantSquare & 0b00111111;
    }

};

vector<Move> GeneratePseudoLegalMoves(Board& board, bool isWhiteToMove) {
    vector<Move> moves;

    if (isWhiteToMove) {
        GenerateSlidingMoves(board.whiteBishops, bishopDirs, std::size(bishopDirs), board.whitePieces, board.blackPieces, moves);
        GenerateSlidingMoves(board.whiteRooks,   rookDirs,   std::size(rookDirs),   board.whitePieces, board.blackPieces, moves);
        GenerateSlidingMoves(board.whiteQueens,  queenDirs,  std::size(queenDirs),  board.whitePieces, board.blackPieces, moves);
    } else {
        GenerateSlidingMoves(board.blackBishops, bishopDirs, std::size(bishopDirs), board.blackPieces, board.whitePieces, moves);
        GenerateSlidingMoves(board.blackRooks,   rookDirs,   std::size(rookDirs),   board.blackPieces, board.whitePieces, moves);
        GenerateSlidingMoves(board.blackQueens,  queenDirs,  std::size(queenDirs),  board.blackPieces, board.whitePieces, moves);
    }

    return moves;
};

void GenerateSlidingMoves(uint64_t pieces, const int directions[][2], int dirCount, uint64_t own, uint64_t enemy, vector<Move>& moves) {
    while (pieces) {
        int sq = __builtin_ctzll(pieces);
        pieces &= pieces - 1;

        int rank = sq / 8;
        int file = sq % 8;

        for (int d = 0; d < dirCount; d++) {
            int r = rank + directions[d][0];
            int f = file + directions[d][1];
            while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                int target = r * 8 + f;
                uint64_t targetBB = 1ULL << target;
                if (own & targetBB) break;
                moves.push_back(Move(sq, target));
                if (enemy & targetBB) break;
                r += directions[d][0];
                f += directions[d][1];
            }
        }
    }
}

string extractFen(const string& input) {
    if (input.rfind("position startpos", 0) == 0) {
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }
    const string prefix = "position fen ";
    size_t start = input.find(prefix);
    if (start == string::npos) return "";

    start += prefix.size();
    size_t movesPos = input.find(" moves ", start);

    if (movesPos == string::npos) {
        return input.substr(start);
    } else {
        return input.substr(start, movesPos - start);
    }
}

int main() {
    cout << "Neptune 1.0 by Jupyter\n";
    cout.flush();

    string line;

    Board board;
    while (getline(cin, line)) {
        if (line == "uci") {
            cout << "id name NeptuneBot" << endl;
            cout << "id author Jupyter" << endl;
            cout << "uciok" << endl << flush;
        } else if (line == "isready") {
            cout << "readyok" << endl << flush;
        } else if (line.rfind("position", 0) == 0) {
            string fen = extractFen(line);
            board.setBB(fen);
            cout << "info string position set" << endl << flush;
        } else if (line.rfind("go", 0) == 0) {
            // find best move (for now, hardcode a move)
            cout << "bestmove e2e4" << endl << flush;
        } else if (line == "quit") {
            break;
        }
    }

    return 0;
}