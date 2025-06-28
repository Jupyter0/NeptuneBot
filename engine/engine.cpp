#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <sstream>

using namespace std;

enum Piece {
    EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Color {
    NONE, WHITE, BLACK
};

struct Square {
    Piece piece;
    Color color;
};

struct Move {
    int from;
    int to;
    char promotion = 0;
};

class Board {
public:
    array<Square, 64> squares;
    uint64_t whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKing;
    uint64_t blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKing;

    uint64_t whitePieces;
    uint64_t blackPieces;
    uint64_t allPieces;

    bool whiteToMove;

    uint8_t castlingRights; //0b0000qkQK q: BQS, k: BKS, Q: WQS, K: WKS
    uint8_t enPassantSquare; //0b0fdddddd f: flag, d: data

    uint8_t halfmoveClock; //Used for 50-move rule
    uint8_t fullmoveNumber; //Counts the move number

    uint64_t zobristKey; //Zobrist Hash

    void setBB(const string& fen) {
        istringstream iss(fen);
        string field;
        vector<string> fields;

        while (iss >> field) {
            fields.push_back(field);
        }

        string pieces = fields[0];
        
    }

    void make_move(const Move& move);
    bool is_king_in_check(bool white);

    void UpdateOccupancy() {
        whitePieces = whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
        blackPieces = blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
        allPieces = whitePieces | blackPieces;
    }
};

vector<Move> GeneratePseudoLegalMoves(Board& board, bool isWhiteToMove) {

};

string extractFen(const string& input) {
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
            cout << "uciok" << endl;
        } else if (line == "isready") {
            cout << "readyok" << endl;
        } else if (line.rfind("position", 0) == 0) {
            string fen = extractFen(line);
            board.setBB(fen);
            cout << "info string position set" << endl;
        } else if (line.rfind("go", 0) == 0) {
            // find best move (for now, hardcode a move)
            cout << "bestmove e2e4" << endl;
        } else if (line == "quit") {
            break;
        }
        cout.flush();
    }

    return 0;
}