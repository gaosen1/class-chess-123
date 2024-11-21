#pragma once
#include "Game.h"
#include "ChessSquare.h"

const int pieceSize = 64;

enum ChessPiece
{
    NoPiece = 0,
    Pawn = 1,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

enum PieceType
{
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

enum PieceColor
{
    WHITE = 0,
    BLACK = 1
};

//
// the main game class
//
class Chess : public Game
{
public:
    Chess();
    ~Chess();

    // set up the board
    void setUpBoard() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;
    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void stopGame() override;
    BitHolder &getHolderAt(const int x, const int y) override { return _grid[y][x]; }

    void updateAI() override;
    bool gameHasAI() override { return true; }

private:
    Bit *PieceForPlayer(const int playerNumber, ChessPiece piece);
    const char bitToPieceNotation(int row, int column) const;

    ChessSquare _grid[8][8];

    void placePiece(int row, int col, PieceType type, PieceColor color);
    bool isValidPosition(int row, int col) const;
    void setupInitialPosition();
    bool validateBoardState() const;

    bool isCurrentPlayersPiece(const Bit &piece) const;
    bool isValidMove(const Bit &piece, const BitHolder &src, const BitHolder &dst) const;
    void highlightLegalMoves(const Bit &piece, const BitHolder &src);
    void clearHighlights();
    bool isPieceBlocking(int startRow, int startCol, int endRow, int endCol) const;
    std::vector<std::pair<int, int>> getLegalMoves(const Bit &piece, int srcRow, int srcCol) const;

    bool _isWhiteTurn = true; // true = 白方回合, false = 黑方回合
};
