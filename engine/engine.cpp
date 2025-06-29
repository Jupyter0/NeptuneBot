#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <sstream>
#include <cctype>
#include <iterator>
#include <random>
#include <chrono>

using namespace std;

constexpr int bishopDirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
constexpr int rookDirs[4][2] = {{0, 1}, {0, -1}, {-1, 0}, {1, 0}};
constexpr int queenDirs[8][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {0, 1}, {0, -1}, {-1, 0}, {1, 0}};
constexpr int knightDirs[8][2] = {{2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}, {2, -1}};
constexpr int kingDirs[8][2] = {{1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {1, 0}};

constexpr std::array<std::array<uint64_t, 64>, 2> pawnAttacks = {{
    // WHITE
    {
        0x0000000000000200ULL, 0x0000000000000500ULL, 0x0000000000000A00ULL, 0x0000000000001400ULL, 
        0x0000000000002800ULL, 0x0000000000005000ULL, 0x000000000000A000ULL, 0x0000000000004000ULL,
        0x0000000000020000ULL, 0x0000000000050000ULL, 0x00000000000A0000ULL, 0x0000000000140000ULL, 
        0x0000000000280000ULL, 0x0000000000500000ULL, 0x0000000000A00000ULL, 0x0000000000400000ULL, 
        0x0000000002000000ULL, 0x0000000005000000ULL, 0x000000000A000000ULL, 0x0000000014000000ULL, 
        0x0000000028000000ULL, 0x0000000050000000ULL, 0x00000000A0000000ULL, 0x0000000040000000ULL, 
        0x0000000200000000ULL, 0x0000000500000000ULL, 0x0000000A00000000ULL, 0x0000001400000000ULL, 
        0x0000002800000000ULL, 0x0000005000000000ULL, 0x000000A000000000ULL, 0x0000004000000000ULL, 
        0x0000020000000000ULL, 0x0000050000000000ULL, 0x00000A0000000000ULL, 0x0000140000000000ULL, 
        0x0000280000000000ULL, 0x0000500000000000ULL, 0x0000A00000000000ULL, 0x0000400000000000ULL, 
        0x0002000000000000ULL, 0x0005000000000000ULL, 0x000A000000000000ULL, 0x0014000000000000ULL, 
        0x0028000000000000ULL, 0x0050000000000000ULL, 0x00A0000000000000ULL, 0x0040000000000000ULL, 
        0x0200000000000000ULL, 0x0500000000000000ULL, 0x0A00000000000000ULL, 0x1400000000000000ULL, 
        0x2800000000000000ULL, 0x5000000000000000ULL, 0xA000000000000000ULL, 0x4000000000000000ULL, 
        0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 
        0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL  
    },

    // BLACK
    {
        0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 
        0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
        0x0000000000000002ULL, 0x0000000000000005ULL, 0x000000000000000AULL, 0x0000000000000014ULL, 
        0x0000000000000028ULL, 0x0000000000000050ULL, 0x00000000000000A0ULL, 0x0000000000000040ULL,
        0x0000000000000200ULL, 0x0000000000000500ULL, 0x0000000000000A00ULL, 0x0000000000001400ULL, 
        0x0000000000002800ULL, 0x0000000000005000ULL, 0x000000000000A000ULL, 0x0000000000004000ULL, 
        0x0000000000020000ULL, 0x0000000000050000ULL, 0x00000000000A0000ULL, 0x0000000000140000ULL, 
        0x0000000000280000ULL, 0x0000000000500000ULL, 0x0000000000A00000ULL, 0x0000000000400000ULL, 
        0x0000000002000000ULL, 0x0000000005000000ULL, 0x000000000A000000ULL, 0x0000000014000000ULL, 
        0x0000000028000000ULL, 0x0000000050000000ULL, 0x00000000A0000000ULL, 0x0000000040000000ULL, 
        0x0000000200000000ULL, 0x0000000500000000ULL, 0x0000000A00000000ULL, 0x0000001400000000ULL, 
        0x0000002800000000ULL, 0x0000005000000000ULL, 0x000000A000000000ULL, 0x0000004000000000ULL, 
        0x0000020000000000ULL, 0x0000050000000000ULL, 0x00000A0000000000ULL, 0x0000140000000000ULL, 
        0x0000280000000000ULL, 0x0000500000000000ULL, 0x0000A00000000000ULL, 0x0000400000000000ULL, 
        0x0002000000000000ULL, 0x0005000000000000ULL, 0x000A000000000000ULL, 0x0014000000000000ULL, 
        0x0028000000000000ULL, 0x0050000000000000ULL, 0x00A0000000000000ULL, 0x0040000000000000ULL
    }
}};
constexpr uint64_t knightAttacks[64] = {
    0x0000000000020400ULL, 0x0000000000050800ULL, 0x00000000000a1100ULL, 0x0000000000142200ULL,
    0x0000000000284400ULL, 0x0000000000508800ULL, 0x0000000000a01000ULL, 0x0000000000402000ULL,
    0x0000000002040004ULL, 0x0000000005080008ULL, 0x000000000a110011ULL, 0x0000000014220022ULL,
    0x0000000028440044ULL, 0x0000000050880088ULL, 0x00000000a0100010ULL, 0x0000000040200020ULL,
    0x0000000204000402ULL, 0x0000000508000805ULL, 0x0000000a1100110aULL, 0x0000001422002214ULL,
    0x0000002844004428ULL, 0x0000005088008850ULL, 0x000000a0100010a0ULL, 0x0000004020002040ULL,
    0x0000020400040200ULL, 0x0000050800080500ULL, 0x00000a1100110a00ULL, 0x0000142200221400ULL,
    0x0000284400442800ULL, 0x0000508800885000ULL, 0x0000a0100010a000ULL, 0x0000402000204000ULL,
    0x0002040004020000ULL, 0x0005080008050000ULL, 0x000a1100110a0000ULL, 0x0014220022140000ULL,
    0x0028440044280000ULL, 0x0050880088500000ULL, 0x00a0100010a00000ULL, 0x0040200020400000ULL,
    0x0204000402000000ULL, 0x0508000805000000ULL, 0x0a1100110a000000ULL, 0x1422002214000000ULL,
    0x2844004428000000ULL, 0x5088008850000000ULL, 0xa0100010a0000000ULL, 0x4020002040000000ULL,
    0x0400040200000000ULL, 0x0800080500000000ULL, 0x1100110a00000000ULL, 0x2200221400000000ULL,
    0x4400442800000000ULL, 0x8800885000000000ULL, 0x100010a000000000ULL, 0x2000204000000000ULL,
    0x0004020000000000ULL, 0x0008050000000000ULL, 0x00110a0000000000ULL, 0x0022140000000000ULL,
    0x0044280000000000ULL, 0x0088500000000000ULL, 0x0010a00000000000ULL, 0x0020400000000000ULL
};
constexpr uint64_t kingAttacks[64] = {
    0x0000000000000302ULL, 0x0000000000000705ULL, 0x0000000000000e0aULL, 0x0000000000001c14ULL,
    0x0000000000003828ULL, 0x0000000000007050ULL, 0x000000000000e0a0ULL, 0x000000000000c040ULL,
    0x0000000000030203ULL, 0x0000000000070507ULL, 0x00000000000e0a0eULL, 0x00000000001c141cULL,
    0x0000000000382838ULL, 0x0000000000705070ULL, 0x0000000000e0a0e0ULL, 0x0000000000c040c0ULL,
    0x0000000003020300ULL, 0x0000000007050700ULL, 0x000000000e0a0e00ULL, 0x000000001c141c00ULL,
    0x0000000038283800ULL, 0x0000000070507000ULL, 0x00000000e0a0e000ULL, 0x00000000c040c000ULL,
    0x0000000302030000ULL, 0x0000000705070000ULL, 0x0000000e0a0e0000ULL, 0x0000001c141c0000ULL,
    0x0000003828380000ULL, 0x0000007050700000ULL, 0x000000e0a0e00000ULL, 0x000000c040c00000ULL,
    0x0000030203000000ULL, 0x0000070507000000ULL, 0x00000e0a0e000000ULL, 0x00001c141c000000ULL,
    0x0000382838000000ULL, 0x0000705070000000ULL, 0x0000e0a0e0000000ULL, 0x0000c040c0000000ULL,
    0x0003020300000000ULL, 0x0007050700000000ULL, 0x000e0a0e00000000ULL, 0x001c141c00000000ULL,
    0x0038283800000000ULL, 0x0070507000000000ULL, 0x00e0a0e000000000ULL, 0x00c040c000000000ULL,
    0x0302030000000000ULL, 0x0705070000000000ULL, 0x0e0a0e0000000000ULL, 0x1c141c0000000000ULL,
    0x3828380000000000ULL, 0x7050700000000000ULL, 0xe0a0e00000000000ULL, 0xc040c00000000000ULL,
    0x0203000000000000ULL, 0x0507000000000000ULL, 0x0a0e000000000000ULL, 0x141c000000000000ULL,
    0x2838000000000000ULL, 0x5070000000000000ULL, 0xa0e0000000000000ULL, 0x40c0000000000000ULL
};

enum Squares {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

constexpr uint64_t castlingBB[4] = {
    (1ULL << f1) | (1ULL << g1),              // White king-side
    (1ULL << b1) | (1ULL << c1) | (1ULL << d1), // White queen-side
    (1ULL << f8) | (1ULL << g8),              // Black king-side
    (1ULL << b8) | (1ULL << c8) | (1ULL << d8)  // Black queen-side
};

constexpr uint64_t rank1 = 0x00000000000000FFULL;
constexpr uint64_t rank2 = 0x000000000000FF00ULL;
constexpr uint64_t rank3 = 0x0000000000FF0000ULL;
constexpr uint64_t rank4 = 0x00000000FF000000ULL;
constexpr uint64_t rank5 = 0x000000FF00000000ULL;
constexpr uint64_t rank6 = 0x0000FF0000000000ULL;
constexpr uint64_t rank7 = 0x00FF000000000000ULL;
constexpr uint64_t rank8 = 0xFF00000000000000ULL;

string indexToSquare(int index) {
    int file = index & 7;  // 0 to 7
    int rank = index >> 3;  // 0 to 7

    char fileChar = 'a' + file;       // 'a' to 'h'
    char rankChar = '1' + rank;       // '1' to '8'

    return string() + fileChar + rankChar;
}

enum Piece {
    EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Color {
    WHITE, BLACK
};

struct Move {
    int from;
    int to;
    char promotion = ' ';
    bool isEnPassant = false;

    Move(int From, int To, char Promotion = 0, bool EP = false) : from(From), to(To), promotion(Promotion), isEnPassant(EP) {}
};

class Board {
public:
    uint64_t whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKing;
    uint64_t blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKing;

    uint64_t whitePieces;
    uint64_t blackPieces;
    uint64_t allPieces;

    uint8_t whiteKingPos;
    uint8_t blackKingPos;

    uint64_t whiteAttacks;
    uint64_t blackAttacks;

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

        ParsePieces(fields[0]);

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

    void UpdateAttacks() {
        whiteAttacks = 0ULL;
        blackAttacks = 0ULL;

        AddNonSlidingAttacks(whiteKnights, KNIGHT, WHITE, whiteAttacks);
        AddNonSlidingAttacks(whiteKing, KING, WHITE, whiteAttacks);
        AddNonSlidingAttacks(whitePawns, PAWN, WHITE, whiteAttacks);
        AddNonSlidingAttacks(blackKnights, KNIGHT, BLACK, blackAttacks);
        AddNonSlidingAttacks(blackKing, KING, BLACK, blackAttacks);
        AddNonSlidingAttacks(blackPawns, PAWN, BLACK, blackAttacks);

        AddSlidingAttacks(whiteBishops, BISHOP, WHITE, whiteAttacks);
        AddSlidingAttacks(whiteRooks, ROOK, WHITE, whiteAttacks);
        AddSlidingAttacks(whiteQueens, QUEEN, WHITE, whiteAttacks);
        AddSlidingAttacks(blackBishops, BISHOP, BLACK, blackAttacks);
        AddSlidingAttacks(blackRooks, ROOK, BLACK, blackAttacks);
        AddSlidingAttacks(blackQueens, QUEEN, BLACK, blackAttacks);
    }
    void AddSlidingAttacks(uint64_t pieces, Piece pieceType, Color color, uint64_t& attacks) {
        if (pieceType == KNIGHT || pieceType == KING || pieceType == PAWN) return;
        if (pieceType == QUEEN) {
            AddSlidingAttacks(pieces, ROOK, color, attacks);
            AddSlidingAttacks(pieces, BISHOP, color, attacks);
            return;
        }
        uint64_t p = pieces;
        while(p) {
            int sq = __builtin_ctzll(p);
            p &= p - 1;

            switch (pieceType) {
                case BISHOP: {
                    attacks |= CalculateSlidingAttacks(sq, bishopDirs, allPieces);
                    break;
                }
                case ROOK: {
                    attacks |= CalculateSlidingAttacks(sq, rookDirs, allPieces);
                    break;
                }
            }
        }
    }

    uint64_t CalculateSlidingAttacks(uint64_t sq, const int directions[][2], uint64_t all) {
        int rank = sq >> 3;
        int file = sq & 7;
        uint64_t attacks = 0;

        for (int d = 0; d < 4; d++) {
            int r = rank + directions[d][0];
            int f = file + directions[d][1];
            while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                int target = r * 8 + f;
                uint64_t targetBB = 1ULL << target;
                attacks |= targetBB;
                if (all & targetBB) break;
                r += directions[d][0];
                f += directions[d][1];
            }           
        }
        return attacks;
    }

    void AddNonSlidingAttacks(uint64_t pieces, Piece pieceType, Color color, uint64_t& attacks) {
        if (pieceType == BISHOP || pieceType == ROOK || pieceType == QUEEN) return;
        uint64_t p = pieces;
        while(p) {
            int sq = __builtin_ctzll(p);
            p &= p - 1;

            switch (pieceType) {
                case KNIGHT: attacks |= knightAttacks[sq]; break;
                case KING: attacks |= kingAttacks[sq]; break;
                case PAWN: attacks |= pawnAttacks[color][sq]; break;
            }
        }
    }

    void ParsePieces(string piecesField) {
        vector<string> ranks;
        size_t start = 0;
        stringstream ss(piecesField);
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
                        default: cerr << "[Warning]: Unknown piece character '" << piece << "'\n"; break;
                }
                file++;
            }
        }
    }

    void make_move(Move move) {
        int from = move.from;
        int to = move.to;
        bool isWhite = whiteToMove;
        uint64_t fromBB = 1ULL << from;
        uint64_t toBB = 1ULL << to;

        // Remove en passant target
        enPassantSquare = 0;

        // Handle piece movement and captures
        auto move_piece = [&](uint64_t& bb, uint64_t from, uint64_t to) {
            bb &= ~from;
            bb |= to;
        };

        // Find and move the correct piece
        uint64_t movedBB = fromBB;
        if (isWhite) {
            if (whitePawns & fromBB) {
                move_piece(whitePawns, fromBB, toBB);

                // En passant capture
                if (move.isEnPassant) {
                    blackPawns &= ~(1ULL << (to - 8));
                }

                // Double pawn push: set en passant target
                if ((fromBB & rank2) && (toBB & rank4)) {
                    enPassantSquare = (1 << 6) | (from + 8); // flag + square
                }

                // Promotion
                if (move.promotion) {
                    whitePawns &= ~toBB;
                    switch (move.promotion) {
                        case 'q': whiteQueens |= toBB; break;
                        case 'r': whiteRooks |= toBB; break;
                        case 'b': whiteBishops |= toBB; break;
                        case 'n': whiteKnights |= toBB; break;
                    }
                }

            } else if (whiteKnights & fromBB) move_piece(whiteKnights, fromBB, toBB);
            else if (whiteBishops & fromBB) move_piece(whiteBishops, fromBB, toBB);
            else if (whiteRooks & fromBB) {
                move_piece(whiteRooks, fromBB, toBB);
                // Remove castling rights if rook moves
                if (from == h1) castlingRights &= ~(1 << 3);
                else if (from == a1) castlingRights &= ~(1 << 2);
            }
            else if (whiteQueens & fromBB) move_piece(whiteQueens, fromBB, toBB);
            else if (whiteKing & fromBB) {
                move_piece(whiteKing, fromBB, toBB);
                whiteKingPos = to;
                castlingRights &= ~(1 << 3 | 1 << 2); // Lose both rights

                // Castling
                if (from == e1 && to == g1) { // King-side
                    move_piece(whiteRooks, 1ULL << h1, 1ULL << f1);
                } else if (from == e1 && to == c1) { // Queen-side
                    move_piece(whiteRooks, 1ULL << a1, 1ULL << d1);
                }
            }

            // Remove captured black piece
            blackPawns   &= ~toBB;
            blackKnights &= ~toBB;
            blackBishops &= ~toBB;
            blackRooks   &= ~toBB;
            blackQueens  &= ~toBB;
            blackKing    &= ~toBB;

        } else { // Black move
            if (blackPawns & fromBB) {
                move_piece(blackPawns, fromBB, toBB);

                if (move.isEnPassant) {
                    whitePawns &= ~(1ULL << (to + 8));
                }

                if ((fromBB & rank7) && (toBB & rank5)) {
                    enPassantSquare = (1 << 6) | (from - 8);
                }

                if (move.promotion) {
                    blackPawns &= ~toBB;
                    switch (move.promotion) {
                        case 'q': blackQueens |= toBB; break;
                        case 'r': blackRooks |= toBB; break;
                        case 'b': blackBishops |= toBB; break;
                        case 'n': blackKnights |= toBB; break;
                    }
                }

            } else if (blackKnights & fromBB) move_piece(blackKnights, fromBB, toBB);
            else if (blackBishops & fromBB) move_piece(blackBishops, fromBB, toBB);
            else if (blackRooks & fromBB) {
                move_piece(blackRooks, fromBB, toBB);
                if (from == h8) castlingRights &= ~(1 << 1);
                else if (from == a8) castlingRights &= ~(1 << 0);
            }
            else if (blackQueens & fromBB) move_piece(blackQueens, fromBB, toBB);
            else if (blackKing & fromBB) {
                move_piece(blackKing, fromBB, toBB);
                blackKingPos = to;
                castlingRights &= ~(1 << 1 | 1 << 0);

                if (from == e8 && to == g8) {
                    move_piece(blackRooks, 1ULL << h8, 1ULL << f8);
                } else if (from == e8 && to == c8) {
                    move_piece(blackRooks, 1ULL << a8, 1ULL << d8);
                }
            }

            whitePawns   &= ~toBB;
            whiteKnights &= ~toBB;
            whiteBishops &= ~toBB;
            whiteRooks   &= ~toBB;
            whiteQueens  &= ~toBB;
            whiteKing    &= ~toBB;
        }

        // Halfmove clock reset
        if ((whitePawns | blackPawns) & toBB || move.isEnPassant ||
            ((whitePieces | blackPieces) & toBB)) {
            halfmoveClock = 0;
        } else {
            halfmoveClock++;
        }

        // Toggle turn
        whiteToMove = !whiteToMove;
        if (!whiteToMove) fullmoveNumber++;

        // Update occupancy and attacks
        UpdateOccupancy();
        UpdateAttacks();
    }



    bool is_king_in_check(bool white) {
        return white ? (blackAttacks & (1ULL << whiteKingPos)) != 0
                    : (whiteAttacks & (1ULL << blackKingPos)) != 0;
    }



    void UpdateOccupancy() {
        whitePieces = whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
        blackPieces = blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
        allPieces = whitePieces | blackPieces;

        whiteKingPos = __builtin_ctzll(whiteKing);
        blackKingPos = __builtin_ctzll(blackKing);
    }

    bool hasEnPassant() const {
        return (enPassantSquare & (1 << 6)) != 0;
    }

    uint8_t getEnPassantTarget() const {
        // Returns 0–63 square index if en passant is available; undefined otherwise
        return enPassantSquare & 0b00111111;
    }

};

void GenerateCastlingMoves(uint64_t king, uint64_t rooks, uint8_t castlingRights, uint64_t all, vector<Move>& moves, Color color) {
    if (color == WHITE) {
        if (((castlingRights & 0b1000) != 0) && ((all & castlingBB[0]) == 0)) {
            moves.push_back(Move(e1, g1));
        }
        if (((castlingRights & 0b0100) != 0) && ((all & castlingBB[1]) == 0)) {
            moves.push_back(Move(e1, c1));
        }
    } else {
        if (((castlingRights & 0b0010) != 0) && ((all & castlingBB[2]) == 0)) {
            moves.push_back(Move(e8, g8));
        }
        if (((castlingRights & 0b0001) != 0) && ((all & castlingBB[3]) == 0)) {
            moves.push_back(Move(e8, c8));
        }
    }
}

void GenerateSlidingMoves(uint64_t pieces, const int directions[][2], int dirCount, uint64_t own, uint64_t enemy, vector<Move>& moves) {
    while (pieces) {
        int sq = __builtin_ctzll(pieces);
        pieces &= pieces - 1;

        int rank = sq >> 3;
        int file = sq & 7;

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

void GenerateNonSlidingMoves(uint64_t pieces, const uint64_t attackTable[64], uint64_t own, vector<Move>& moves) {
    while (pieces) {
        int sq = __builtin_ctzll(pieces);
        pieces &= pieces - 1;

        uint64_t attacks = attackTable[sq];
        attacks &= ~own;

        while (attacks) {
            int target = __builtin_ctzll(attacks);
            attacks &= attacks - 1;
            moves.push_back(Move(sq, target));
        }
    }
}


void GeneratePawnMoves(uint64_t pawns, uint64_t enemy, uint64_t all, vector<Move>& moves, Color color, uint8_t epSquare = 64) {
    int forward = (color == WHITE) ? 8 : -8;
    int startRank = (color == WHITE) ? 1 : 6;
    int promotionRank = (color == WHITE) ? 6 : 1;

    while (pawns) {
        int sq = __builtin_ctzll(pawns);
        pawns &= pawns - 1;

        int rank = sq >> 3;

        int oneStep = sq + forward;
        if (oneStep >= 0 && oneStep < 64 && !(all & (1ULL << oneStep))) {
            if (rank == promotionRank) {
                moves.push_back(Move(sq, oneStep, 'q'));
                moves.push_back(Move(sq, oneStep, 'r'));
                moves.push_back(Move(sq, oneStep, 'b'));
                moves.push_back(Move(sq, oneStep, 'n'));
            } else {
                moves.push_back(Move(sq, oneStep));

                if (rank == startRank) {
                    int twoStep = sq + 2 * forward;
                    if (!(all & (1ULL << twoStep))) {
                        moves.push_back(Move(sq, twoStep));
                    }
                }
            }
        }

        uint64_t attacks = pawnAttacks[color][sq];
        uint64_t normalCaptures = attacks & enemy;

        while (normalCaptures) {
            int targetSq = __builtin_ctzll(normalCaptures);
            normalCaptures &= normalCaptures - 1;

            if (rank == promotionRank) {
                moves.push_back(Move(sq, targetSq, 'q'));
                moves.push_back(Move(sq, targetSq, 'r'));
                moves.push_back(Move(sq, targetSq, 'b'));
                moves.push_back(Move(sq, targetSq, 'n'));
            } else {
                moves.push_back(Move(sq, targetSq));
            }
        }

        if (epSquare < 64) {
            uint64_t epBB = 1ULL << epSquare;
            if ((attacks & epBB) != 0) {
                moves.push_back(Move(sq, epSquare, 0, true));
            }
        }
    }
}

vector<Move> GeneratePseudoLegalMoves(Board& board, bool isWhiteToMove) {
    vector<Move> moves;
    moves.reserve(256);
    uint8_t epTarget = board.hasEnPassant() ? board.getEnPassantTarget() : 64;

    if (isWhiteToMove) {
        GenerateSlidingMoves(board.whiteBishops, bishopDirs, std::size(bishopDirs), board.whitePieces, board.blackPieces, moves);
        GenerateSlidingMoves(board.whiteRooks,   rookDirs,   std::size(rookDirs),   board.whitePieces, board.blackPieces, moves);
        GenerateSlidingMoves(board.whiteQueens,  queenDirs,  std::size(queenDirs),  board.whitePieces, board.blackPieces, moves);
        GenerateNonSlidingMoves(board.whiteKnights, knightAttacks, board.whitePieces, moves);
        GenerateNonSlidingMoves(board.whiteKing, kingAttacks, board.whitePieces, moves);
        GeneratePawnMoves(board.whitePawns, board.blackPieces, board.allPieces, moves, WHITE, epTarget);
        GenerateCastlingMoves(board.whiteKing, board.whiteRooks, board.castlingRights, board.allPieces, moves, WHITE);
    } else {
        GenerateSlidingMoves(board.blackBishops, bishopDirs, std::size(bishopDirs), board.blackPieces, board.whitePieces, moves);
        GenerateSlidingMoves(board.blackRooks,   rookDirs,   std::size(rookDirs),   board.blackPieces, board.whitePieces, moves);
        GenerateSlidingMoves(board.blackQueens,  queenDirs,  std::size(queenDirs),  board.blackPieces, board.whitePieces, moves);
        GenerateNonSlidingMoves(board.blackKnights, knightAttacks, board.blackPieces, moves);
        GenerateNonSlidingMoves(board.blackKing, kingAttacks, board.blackPieces, moves);
        GeneratePawnMoves(board.blackPawns, board.whitePieces, board.allPieces, moves, BLACK, epTarget);
        GenerateCastlingMoves(board.blackKing, board.blackRooks, board.castlingRights, board.allPieces, moves, BLACK);
    }

    return moves;
};

vector<Move> GenerateLegalMoves(Board& board) {
    vector<Move> pseudoMoves = GeneratePseudoLegalMoves(board, board.whiteToMove);
    vector<Move> legalMoves;
    legalMoves.reserve(pseudoMoves.size());

    for (const Move& move : pseudoMoves) {
        Board temp = board;
        cout << "--- DEBUG ---" << endl << flush;
        cout << "Move: " << indexToSquare(move.from) << indexToSquare(move.to) << move.promotion << endl << flush;
        cout << "Pieces premove: " << temp.allPieces << endl << flush;
        temp.make_move(move);
        cout << "Pieces postmove: " << temp.allPieces << endl << flush;
        cout << "White ATK: " << temp.whiteAttacks << endl << flush;
        cout << "Black ATK: " << temp.blackAttacks << endl << flush;
        cout << endl << flush;
        if (!temp.is_king_in_check(!temp.whiteToMove)) { // check if own king is not in check
            legalMoves.push_back(move);
        }
    }

    return legalMoves;
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
            cout << "read fen " << fen << endl << flush;
        } else if (line.rfind("go", 0) == 0) {
            vector<Move> legalMoves = GenerateLegalMoves(board);
            cout << "found " << legalMoves.size() << " legal moves" << endl << flush;
            cout << "Legal Moves: ";
            for (const auto& move : legalMoves) {
                string promotion = move.promotion == ' ' ? "" : string(1, move.promotion);
                cout << indexToSquare(move.from) << indexToSquare(move.to) << promotion << " ";
            }
            std::cout << std::endl;
            std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<int> dist(0, legalMoves.size() - 1);
            int randomIndex = dist(engine);
            Move bestMove = legalMoves[randomIndex];
            string promotion = bestMove.promotion == ' ' ? "" : string(1, bestMove.promotion);
            string bestUCI = indexToSquare(bestMove.from) + indexToSquare(bestMove.to) + promotion;
            cout << "bestmove " << bestUCI << endl << flush;
        } else if (line == "quit") {
            break;
        }
    }

    return 0;
}