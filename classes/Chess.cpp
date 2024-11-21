#include "Chess.h"

const int AI_PLAYER = 1;
const int HUMAN_PLAYER = -1;

Chess::Chess()
{
}

Chess::~Chess()
{
}

//
// make a chess piece for the player
//
Bit *Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char *pieces[] = {"pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png"};

    // depending on playerNumber load the "x.png" or the "o.png" graphic
    Bit *bit = new Bit();
    // should possibly be cached from player class?
    const char *pieceName = pieces[piece - 1];
    std::string spritePath = std::string("chess/") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    // 初始化棋盘格子
    for (int y = 0; y < _gameOptions.rowY; y++)
    {
        for (int x = 0; x < _gameOptions.rowX; x++)
        {
            ImVec2 position((float)(pieceSize * x + pieceSize),
                            (float)(pieceSize * (_gameOptions.rowY - y) + pieceSize));
            _grid[y][x].initHolder(position, "boardsquare.png", x, y);
            _grid[y][x].setGameTag(0);

            // 设置棋盘表示法
            char notation[3];
            notation[0] = 'a' + x;
            notation[1] = '1' + y;
            notation[2] = 0;
            _grid[y][x].setNotation(notation);
        }
    }

    // 设置初始棋盘位置
    setupInitialPosition();

    // 验证棋盘状态
    if (!validateBoardState())
    {
        // 处理错误状态
        printf("Invalid board state detected!\n");
    }
}

//
// about the only thing we need to actually fill out for tic-tac-toe
//
bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    if (!isCurrentPlayersPiece(bit))
    {
        return false;
    }
    highlightLegalMoves(bit, src);
    return true;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    if (!isCurrentPlayersPiece(bit))
    {
        return false;
    }

    // 获取源位置和目标位置的行列
    int srcRow = -1, srcCol = -1, dstRow = -1, dstCol = -1;
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (&_grid[row][col] == &src)
            {
                srcRow = row;
                srcCol = col;
            }
            if (&_grid[row][col] == &dst)
            {
                dstRow = row;
                dstCol = col;
            }
        }
    }

    if (srcRow == -1 || dstRow == -1)
        return false;

    auto legalMoves = getLegalMoves(bit, srcRow, srcCol);
    return std::find(legalMoves.begin(), legalMoves.end(),
                     std::make_pair(dstRow, dstCol)) != legalMoves.end();
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    Game::bitMovedFromTo(bit, src, dst);
    _isWhiteTurn = !_isWhiteTurn; // 切换回合
    clearHighlights();
}

//
// free all the memory used by the game on the heap
//
void Chess::stopGame()
{
}

Player *Chess::checkForWinner()
{
    // check to see if either player has won
    return nullptr;
}

bool Chess::checkForDraw()
{
    // check to see if the board is full
    return false;
}

//
// add a helper to Square so it returns out FEN chess notation in the form p for white pawn, K for black king, etc.
// this version is used from the top level board to record moves
//
const char Chess::bitToPieceNotation(int row, int column) const
{
    if (row < 0 || row >= 8 || column < 0 || column >= 8)
    {
        return '0';
    }

    const char *wpieces = {"?PNBRQK"};
    const char *bpieces = {"?pnbrqk"};
    unsigned char notation = '0';
    Bit *bit = _grid[row][column].bit();
    if (bit)
    {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag() & 127];
    }
    else
    {
        notation = '0';
    }
    return notation;
}

//
// state strings
//
std::string Chess::initialStateString()
{
    return stateString();
}

//
// this still needs to be tied into imguis init and shutdown
// we will read the state string and store it in each turn object
//
std::string Chess::stateString()
{
    std::string s;
    for (int y = 0; y < _gameOptions.rowY; y++)
    {
        for (int x = 0; x < _gameOptions.rowX; x++)
        {
            s += bitToPieceNotation(y, x);
        }
    }
    return s;
}

//
// this still needs to be tied into imguis init and shutdown
// when the program starts it will load the current game from the imgui ini file and set the game state to the last saved state
//
void Chess::setStateString(const std::string &s)
{
    for (int y = 0; y < _gameOptions.rowY; y++)
    {
        for (int x = 0; x < _gameOptions.rowX; x++)
        {
            int index = y * _gameOptions.rowX + x;
            int playerNumber = s[index] - '0';
            if (playerNumber)
            {
                _grid[y][x].setBit(PieceForPlayer(playerNumber - 1, Pawn));
            }
            else
            {
                _grid[y][x].setBit(nullptr);
            }
        }
    }
}

//
// this is the function that will be called by the AI
//
void Chess::updateAI()
{
}

void Chess::placePiece(int row, int col, PieceType type, PieceColor color)
{
    if (!isValidPosition(row, col))
    {
        return;
    }

    // 计算piece tag: 使用高位区分颜色,低位存储类型
    int pieceTag = type | (color == BLACK ? 128 : 0);

    Bit *piece = PieceForPlayer(color, (ChessPiece)type);
    if (piece)
    {
        piece->setGameTag(pieceTag);
        _grid[row][col].setBit(piece);
        piece->setPosition(_grid[row][col].getPosition());
    }
}

bool Chess::isValidPosition(int row, int col) const
{
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

void Chess::setupInitialPosition()
{
    // 放置白方棋子
    // 第一排
    placePiece(0, 0, ROOK, WHITE);
    placePiece(0, 1, KNIGHT, WHITE);
    placePiece(0, 2, BISHOP, WHITE);
    placePiece(0, 3, QUEEN, WHITE);
    placePiece(0, 4, KING, WHITE);
    placePiece(0, 5, BISHOP, WHITE);
    placePiece(0, 6, KNIGHT, WHITE);
    placePiece(0, 7, ROOK, WHITE);

    // 白方兵
    for (int col = 0; col < 8; col++)
    {
        placePiece(1, col, PAWN, WHITE);
    }

    // 放置黑方棋子
    // 最后一排
    placePiece(7, 0, ROOK, BLACK);
    placePiece(7, 1, KNIGHT, BLACK);
    placePiece(7, 2, BISHOP, BLACK);
    placePiece(7, 3, QUEEN, BLACK);
    placePiece(7, 4, KING, BLACK);
    placePiece(7, 5, BISHOP, BLACK);
    placePiece(7, 6, KNIGHT, BLACK);
    placePiece(7, 7, ROOK, BLACK);

    // 黑方兵
    for (int col = 0; col < 8; col++)
    {
        placePiece(6, col, PAWN, BLACK);
    }
}

bool Chess::validateBoardState() const
{
    int whiteKings = 0;
    int blackKings = 0;

    // 检查基本规则
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Bit *piece = _grid[row][col].bit();
            if (piece)
            {
                int tag = piece->gameTag();
                bool isBlack = (tag & 128) != 0;
                int type = tag & 127;

                // 检查王的数量
                if (type == KING)
                {
                    if (isBlack)
                        blackKings++;
                    else
                        whiteKings++;
                }

                // 检查兵的位置
                if (type == PAWN)
                {
                    if ((isBlack && row == 0) || (!isBlack && row == 7))
                    {
                        return false; // 兵不能在第一行或最后一行
                    }
                }
            }
        }
    }

    // 每方必须有且仅有一个王
    return whiteKings == 1 && blackKings == 1;
}

bool Chess::isCurrentPlayersPiece(const Bit &piece) const
{
    bool isPieceWhite = (piece.gameTag() & 128) == 0;
    return isPieceWhite == _isWhiteTurn;
}

bool Chess::isPieceBlocking(int startRow, int startCol, int endRow, int endCol) const
{
    int rowStep = (endRow - startRow) ? (endRow - startRow) / abs(endRow - startRow) : 0;
    int colStep = (endCol - startCol) ? (endCol - startCol) / abs(endCol - startCol) : 0;

    int row = startRow + rowStep;
    int col = startCol + colStep;

    while (row != endRow || col != endCol)
    {
        if (_grid[row][col].bit() != nullptr)
        {
            return true;
        }
        row += rowStep;
        col += colStep;
    }
    return false;
}

std::vector<std::pair<int, int>> Chess::getLegalMoves(const Bit &piece, int srcRow, int srcCol) const
{
    std::vector<std::pair<int, int>> moves;
    int pieceType = piece.gameTag() & 127;
    bool isBlack = (piece.gameTag() & 128) != 0;

    switch (pieceType)
    {
    case PAWN:
    {
        int direction = isBlack ? -1 : 1;
        int startRow = isBlack ? 6 : 1;

        // 前进一步
        if (isValidPosition(srcRow + direction, srcCol) &&
            !_grid[srcRow + direction][srcCol].bit())
        {
            moves.push_back({srcRow + direction, srcCol});

            // 初始位置可以走两步
            if (srcRow == startRow && !_grid[srcRow + 2 * direction][srcCol].bit())
            {
                moves.push_back({srcRow + 2 * direction, srcCol});
            }
        }

        // 吃子移动
        for (int dc : {-1, 1})
        {
            int newCol = srcCol + dc;
            int newRow = srcRow + direction;
            if (isValidPosition(newRow, newCol))
            {
                Bit *targetPiece = _grid[newRow][newCol].bit();
                if (targetPiece && ((targetPiece->gameTag() & 128) != 0) != isBlack)
                {
                    moves.push_back({newRow, newCol});
                }
            }
        }
        break;
    }

    case KNIGHT:
    {
        const int knightMoves[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
        for (auto &move : knightMoves)
        {
            int newRow = srcRow + move[0];
            int newCol = srcCol + move[1];
            if (isValidPosition(newRow, newCol))
            {
                Bit *targetPiece = _grid[newRow][newCol].bit();
                if (!targetPiece || ((targetPiece->gameTag() & 128) != 0) != isBlack)
                {
                    moves.push_back({newRow, newCol});
                }
            }
        }
        break;
    }

        // 其他棋子的移动规则类似实现...
    }

    return moves;
}

void Chess::highlightLegalMoves(const Bit &piece, const BitHolder &src)
{
    clearHighlights();

    if (!isCurrentPlayersPiece(piece))
    {
        return;
    }

    // 获取源位置的行列
    int srcRow = -1, srcCol = -1;
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (&_grid[row][col] == &src)
            {
                srcRow = row;
                srcCol = col;
                break;
            }
        }
    }

    if (srcRow == -1)
        return;

    auto legalMoves = getLegalMoves(piece, srcRow, srcCol);
    for (const auto &move : legalMoves)
    {
        _grid[move.first][move.second].setMoveHighlighted(true);
    }
}

void Chess::clearHighlights()
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            _grid[row][col].setMoveHighlighted(false);
        }
    }
}
