#pragma once
#include "Game.h"
#include "ChessSquare.h"
#include "../imgui/imgui.h"

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

    bool isCheckmate(bool blackKing) const;
    void promotePawn(int row, int col);

    void drawFrame() override;

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

    // 将军检测相关
    bool isSquareUnderAttack(int row, int col, bool byBlack, bool checkKing = true) const;
    bool isInCheck(bool blackKing) const;
    std::pair<int, int> getKingPosition(bool blackKing) const;

    // 王车易位相关
    struct CastlingRights
    {
        bool whiteKingMoved = false;
        bool blackKingMoved = false;
        bool whiteRookKingside = false;
        bool whiteRookQueenside = false;
        bool blackRookKingside = false;
        bool blackRookQueenside = false;
    } _castlingRights;

    bool canCastle(bool kingSide, bool isBlack) const;
    void updateCastlingRights(const Bit &piece, int fromRow, int fromCol);

    std::vector<std::pair<int, int>> getBasicLegalMoves(const Bit &piece, int srcRow, int srcCol, bool ignorePinned) const;

    // 添加 GameStatus 结构体定义
    struct GameStatus
    {
        bool showGameEndPopup = false;
        bool showCheckPopup = false;
        bool showCapturePopup = false;
        std::string statusMessage;
        float popupTimer = 0.0f;
        const float POPUP_DURATION = 2.0f;
    } _gameStatus;

    // 添加 renderGameStatus 函数声明
    void renderGameStatus();

    // 添加结构体定义
    struct LastMoveState
    {
        Bit *targetPiece;
        bool isCapture;
        LastMoveState() : targetPiece(nullptr), isCapture(false) {}
    };

    // 添加成员变量
    LastMoveState _lastMoveState;

    // 只声明静态成员变量，不要在这里初始化
    static int lastDstRow;
    static int lastDstCol;
};
