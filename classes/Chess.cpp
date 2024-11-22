#include "Chess.h"

// 在这里初始化静态成员变量
int Chess::lastDstRow = -1;
int Chess::lastDstCol = -1;

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
    bool isBlack = (bit.gameTag() & 128) != 0;
    if (isBlack == _isWhiteTurn) // 如果不是当前玩家的回合
    {
        return false;
    }

    if (!isCurrentPlayersPiece(bit))
    {
        return false;
    }
    highlightLegalMoves(bit, src);
    return true;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    bool isBlack = (bit.gameTag() & 128) != 0;
    if (isBlack == _isWhiteTurn)
    {
        return false;
    }

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

    // 获取目标位置的棋子
    Bit *targetPiece = _grid[dstRow][dstCol].bit();

    // 只在目标位置改变时才打印信息
    static int lastPrintedDstRow = -1;
    static int lastPrintedDstCol = -1;
    if (dstRow != lastPrintedDstRow || dstCol != lastPrintedDstCol)
    {
        printf("\n=== Checking Move ===\n");
        printf("Source: [%d,%d], Destination: [%d,%d]\n", srcRow, srcCol, dstRow, dstCol);

        if (targetPiece)
        {
            printf("Target square has: %s %s\n",
                   ((targetPiece->gameTag() & 128) != 0) ? "black" : "white",
                   (targetPiece->gameTag() & 127) == PAWN ? "pawn" : (targetPiece->gameTag() & 127) == KNIGHT ? "knight"
                                                                 : (targetPiece->gameTag() & 127) == BISHOP   ? "bishop"
                                                                 : (targetPiece->gameTag() & 127) == ROOK     ? "rook"
                                                                 : (targetPiece->gameTag() & 127) == QUEEN    ? "queen"
                                                                 : (targetPiece->gameTag() & 127) == KING     ? "king"
                                                                                                              : "unknown");
        }
        else
        {
            printf("Target square is empty\n");
        }

        lastPrintedDstRow = dstRow;
        lastPrintedDstCol = dstCol;
    }

    // 保存目标位置的状态
    _lastMoveState.targetPiece = targetPiece;
    if (targetPiece)
    {
        _lastMoveState.isCapture = ((targetPiece->gameTag() & 128) != 0) != isBlack;
    }
    else
    {
        _lastMoveState.isCapture = false;
    }

    lastDstRow = dstRow;
    lastDstCol = dstCol;

    auto legalMoves = getLegalMoves(bit, srcRow, srcCol);
    bool isLegal = std::find(legalMoves.begin(), legalMoves.end(),
                             std::make_pair(dstRow, dstCol)) != legalMoves.end();

    if (dstRow != lastDstRow || dstCol != lastDstCol)
    {
        printf("Move is %s\n", isLegal ? "legal" : "illegal");
        printf("=== Check End ===\n\n");
    }

    return isLegal;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    printf("\n=== Move Start ===\n");

    // 获取源位置和目标位置
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

    bool isBlack = (bit.gameTag() & 128) != 0;
    int pieceType = bit.gameTag() & 127;

    // 使用保存的状态
    Bit *targetPiece = _lastMoveState.targetPiece;
    bool isCapture = _lastMoveState.isCapture;

    printf("Moving %s %s from [%d,%d] to [%d,%d]\n",
           isBlack ? "black" : "white",
           pieceType == PAWN ? "pawn" : pieceType == KNIGHT ? "knight"
                                    : pieceType == BISHOP   ? "bishop"
                                    : pieceType == ROOK     ? "rook"
                                    : pieceType == QUEEN    ? "queen"
                                    : pieceType == KING     ? "king"
                                                            : "unknown",
           srcRow, srcCol, dstRow, dstCol);

    if (isCapture)
    {
        int capturedType = targetPiece->gameTag() & 127;
        printf("Capturing %s %s\n",
               ((targetPiece->gameTag() & 128) != 0) ? "black" : "white",
               capturedType == PAWN ? "pawn" : capturedType == KNIGHT ? "knight"
                                           : capturedType == BISHOP   ? "bishop"
                                           : capturedType == ROOK     ? "rook"
                                           : capturedType == QUEEN    ? "queen"
                                           : capturedType == KING     ? "king"
                                                                      : "unknown");

        if (capturedType == KING)
        {
            printf("\n=== Game Over ===\n");
            printf("King captured! %s wins!\n", isBlack ? "Black" : "White");
            _gameStatus.showGameEndPopup = true;
            _gameStatus.statusMessage = std::string(isBlack ? "Black" : "White") +
                                        " wins by capturing the " +
                                        std::string(!isBlack ? "Black" : "White") +
                                        " King!";
            _isWhiteTurn = !_isWhiteTurn;
            return;
        }
    }

    // 执行移动
    dst.setBit(&bit);
    bit.setPosition(dst.getPosition());

    // 切换回合
    _isWhiteTurn = !_isWhiteTurn;
    printf("Turn changed to %s\n", _isWhiteTurn ? "White" : "Black");

    // 检查对手是否被将军
    bool opponentIsBlack = !_isWhiteTurn;

    // 只在移动完成后检查一次将军状态
    if (isInCheck(opponentIsBlack))
    {
        // 检查是否将死
        bool hasLegalMove = false;

        // 遍历对手所有棋子
        for (int r = 0; r < 8 && !hasLegalMove; r++)
        {
            for (int c = 0; c < 8 && !hasLegalMove; c++)
            {
                Bit *piece = _grid[r][c].bit();
                if (!piece || ((piece->gameTag() & 128) != 0) != opponentIsBlack)
                    continue;

                // 只要找到一个合法移动就可以退出
                if (!getLegalMoves(*piece, r, c).empty())
                {
                    hasLegalMove = true;
                    break;
                }
            }
        }

        if (!hasLegalMove)
        {
            printf("\n=== Game Over ===\n");
            printf("Checkmate! %s wins!\n", !opponentIsBlack ? "White" : "Black");
            _gameStatus.showGameEndPopup = true;
            _gameStatus.statusMessage = std::string(!opponentIsBlack ? "White" : "Black") +
                                        " wins by checkmate!";
        }
        else
        {
            _gameStatus.showCheckPopup = true;
            _gameStatus.popupTimer = _gameStatus.POPUP_DURATION;
        }
    }

    printf("=== Move End ===\n\n");
    clearHighlights();

    // 结束回合
    endTurn();
}

//
// free all the memory used by the game on the heap
//
void Chess::stopGame()
{
}

Player *Chess::checkForWinner()
{
    if (isCheckmate(true))
    {
        return getPlayerAt(0); // 白方获胜
    }
    if (isCheckmate(false))
    {
        return getPlayerAt(1); // 黑方获胜
    }
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
    // 首清除所有现有的棋子
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (_grid[row][col].bit())
            {
                _grid[row][col].destroyBit(); // 删除棋子对象
            }
            _grid[row][col].setBit(nullptr);
        }
    }

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
                        return false; // 兵不能在一行或最后一行
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

std::vector<std::pair<int, int>> Chess::getBasicLegalMoves(const Bit &piece, int srcRow, int srcCol, bool ignorePinned) const
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

        // 前进一
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
    case BISHOP:
    {
        // 象的移动 - 斜线方向
        const int directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        for (auto &dir : directions)
        {
            for (int i = 1; i < 8; i++)
            {
                int newRow = srcRow + dir[0] * i;
                int newCol = srcCol + dir[1] * i;
                if (!isValidPosition(newRow, newCol))
                    break;

                Bit *targetPiece = _grid[newRow][newCol].bit();
                if (!targetPiece)
                {
                    moves.push_back({newRow, newCol});
                }
                else
                {
                    if (((targetPiece->gameTag() & 128) != 0) != isBlack)
                    {
                        moves.push_back({newRow, newCol}); // 可以吃子
                    }
                    break; // 无论是否可以吃子，都不能继续前进
                }
            }
        }
        break;
    }
    case ROOK:
    {
        // 车的移动 - 直线方向
        const int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (auto &dir : directions)
        {
            for (int i = 1; i < 8; i++)
            {
                int newRow = srcRow + dir[0] * i;
                int newCol = srcCol + dir[1] * i;
                if (!isValidPosition(newRow, newCol))
                    break;

                Bit *targetPiece = _grid[newRow][newCol].bit();
                if (!targetPiece)
                {
                    moves.push_back({newRow, newCol});
                }
                else
                {
                    if (((targetPiece->gameTag() & 128) != 0) != isBlack)
                    {
                        moves.push_back({newRow, newCol}); // 可以吃子
                    }
                    break; // 无论是否可以吃子，都不能继续前进
                }
            }
        }
        break;
    }
    case QUEEN:
    {
        // 后的动 - 合象和车的动
        const int directions[8][2] = {
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}, // 斜线方向(象)
            {-1, 0},
            {1, 0},
            {0, -1},
            {0, 1} // 直线方向(车)
        };
        for (auto &dir : directions)
        {
            for (int i = 1; i < 8; i++)
            {
                int newRow = srcRow + dir[0] * i;
                int newCol = srcCol + dir[1] * i;
                if (!isValidPosition(newRow, newCol))
                    break;

                Bit *targetPiece = _grid[newRow][newCol].bit();
                if (!targetPiece)
                {
                    moves.push_back({newRow, newCol});
                }
                else
                {
                    if (((targetPiece->gameTag() & 128) != 0) != isBlack)
                    {
                        moves.push_back({newRow, newCol}); // 可以吃子
                    }
                    break; // 无论是否可以吃子，都不继续前进
                }
            }
        }
        break;
    }
    case KING:
    {
        // 王的移动 - 周围一格
        const int directions[8][2] = {
            {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

        for (auto &dir : directions)
        {
            int newRow = srcRow + dir[0];
            int newCol = srcCol + dir[1];

            if (!isValidPosition(newRow, newCol))
                continue;

            Bit *targetPiece = _grid[newRow][newCol].bit();

            // 检查目标位置是否有己方棋子
            if (targetPiece && ((targetPiece->gameTag() & 128) != 0) == isBlack)
                continue;

            // 检查是否会与对方的王相邻
            auto [enemyKingRow, enemyKingCol] = getKingPosition(!isBlack);
            if (abs(enemyKingRow - newRow) <= 1 && abs(enemyKingCol - newCol) <= 1)
                continue; // 允许两个王相

            moves.push_back({newRow, newCol});
        }
        break;
    }
    }

    return moves;
}

bool Chess::isSquareUnderAttack(int row, int col, bool byBlack, bool checkKing) const
{
    // 首先检查对方王的攻击范围（即使 checkKing 为 false）
    auto [enemyKingRow, enemyKingCol] = getKingPosition(byBlack);
    if (enemyKingRow != -1)
    { // 如果找到了敌方的王
        // 检查目标格子是否在敌方王的攻击范围内（相邻格子）
        if (abs(enemyKingRow - row) <= 1 && abs(enemyKingCol - col) <= 1)
        {
            return true;
        }
    }

    // 然后检查其他棋子的攻击
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Bit *piece = _grid[r][c].bit();
            if (!piece)
                continue;

            // 跳过己方棋子
            if (((piece->gameTag() & 128) != 0) != byBlack)
                continue;

            // 如果不检查王的攻击，跳过王
            if (!checkKing && (piece->gameTag() & 127) == KING)
                continue;

            // 获取基本移动，避免递归
            auto moves = getBasicLegalMoves(*piece, r, c, true);
            if (std::find(moves.begin(), moves.end(),
                          std::make_pair(row, col)) != moves.end())
            {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::pair<int, int>> Chess::getLegalMoves(const Bit &piece, int srcRow, int srcCol) const
{
    bool isBlack = (piece.gameTag() & 128) != 0;
    int pieceType = piece.gameTag() & 127;

    // 获取基本移动
    auto moves = getBasicLegalMoves(piece, srcRow, srcCol, false);

    // 如是王，需要过滤掉不安全的移动
    if (pieceType == KING)
    {
        moves.erase(
            std::remove_if(moves.begin(), moves.end(),
                           [this, isBlack](const std::pair<int, int> &move)
                           {
                               // 检查目标位置是否被攻击，不考虑对方王的攻击
                               return isSquareUnderAttack(move.first, move.second, !isBlack, false);
                           }),
            moves.end());

        // 在这里检查王车易位
        if (!isInCheck(isBlack))
        {
            if (canCastle(true, isBlack))
            {
                moves.push_back({srcRow, srcCol + 2});
            }
            if (canCastle(false, isBlack))
            {
                moves.push_back({srcRow, srcCol - 2});
            }
        }
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

std::pair<int, int> Chess::getKingPosition(bool blackKing) const
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Bit *piece = _grid[row][col].bit();
            if (piece && (piece->gameTag() & 127) == KING &&
                ((piece->gameTag() & 128) != 0) == blackKing)
            {
                return {row, col};
            }
        }
    }
    return {-1, -1}; // 不应该发生
}

bool Chess::isInCheck(bool blackKing) const
{
    auto [kingRow, kingCol] = getKingPosition(blackKing);
    if (kingRow == -1 || kingCol == -1)
    {
        return false;
    }

    // 检查是否有任何敌方棋子可以攻击到王的位置
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Bit *piece = _grid[r][c].bit();
            if (!piece || ((piece->gameTag() & 128) != 0) == blackKing)
                continue;

            // 获取这个棋子的基本移动
            auto moves = getBasicLegalMoves(*piece, r, c, true);
            if (std::find(moves.begin(), moves.end(),
                          std::make_pair(kingRow, kingCol)) != moves.end())
            {
                return true;
            }
        }
    }
    return false;
}

bool Chess::canCastle(bool kingSide, bool isBlack) const
{
    // 检查王和车是否移动过
    if (isBlack)
    {
        if (_castlingRights.blackKingMoved)
            return false;
        if (kingSide && _castlingRights.blackRookKingside)
            return false;
        if (!kingSide && _castlingRights.blackRookQueenside)
            return false;
    }
    else
    {
        if (_castlingRights.whiteKingMoved)
            return false;
        if (kingSide && _castlingRights.whiteRookKingside)
            return false;
        if (!kingSide && _castlingRights.whiteRookQueenside)
            return false;
    }

    int row = isBlack ? 7 : 0;
    int kingCol = 4;

    // 检查王是否正在被将军
    if (isInCheck(isBlack))
        return false;

    // 检查路径上是否有棋子
    if (kingSide)
    {
        for (int col = 5; col < 7; col++)
        {
            if (_grid[row][col].bit() != nullptr)
                return false;
            // 检查路径上的格子是否被攻击
            if (isSquareUnderAttack(row, col, !isBlack))
                return false;
        }
    }
    else
    {
        for (int col = 3; col > 0; col--)
        {
            if (_grid[row][col].bit() != nullptr)
                return false;
            // 检查路径上的格子是否被攻击
            if (isSquareUnderAttack(row, col, !isBlack))
                return false;
        }
    }

    return true;
}

void Chess::updateCastlingRights(const Bit &piece, int fromRow, int fromCol)
{
    int pieceType = piece.gameTag() & 127;
    bool isBlack = (piece.gameTag() & 128) != 0;

    if (pieceType == KING)
    {
        if (isBlack)
            _castlingRights.blackKingMoved = true;
        else
            _castlingRights.whiteKingMoved = true;
    }
    else if (pieceType == ROOK)
    {
        if (isBlack)
        {
            if (fromCol == 0)
                _castlingRights.blackRookQueenside = true;
            if (fromCol == 7)
                _castlingRights.blackRookKingside = true;
        }
        else
        {
            if (fromCol == 0)
                _castlingRights.whiteRookQueenside = true;
            if (fromCol == 7)
                _castlingRights.whiteRookKingside = true;
        }
    }
}

bool Chess::isCheckmate(bool blackKing) const
{
    // 如果不在将军状态，就不可能是将
    if (!isInCheck(blackKing))
    {
        return false;
    }

    // 检查所有己方棋子的所有可能移动
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Bit *piece = _grid[row][col].bit();
            if (!piece)
                continue;

            // 只检查己方棋
            if (((piece->gameTag() & 128) != 0) != blackKing)
                continue;

            // 获取所有合法移动
            auto moves = getLegalMoves(*piece, row, col);
            if (!moves.empty())
            {
                return false; // 只要有一个合法移动，就不是将死
            }
        }
    }

    return true; // 没有任何合法移动，是将死
}

void Chess::promotePawn(int row, int col)
{
    Bit *piece = _grid[row][col].bit();
    if (!piece || (piece->gameTag() & 127) != PAWN)
        return;

    bool isBlack = (piece->gameTag() & 128) != 0;
    // 检查是否到达对方底线
    if ((isBlack && row == 0) || (!isBlack && row == 7))
    {
        // 默认升变为皇后
        delete piece;
        placePiece(row, col, QUEEN, isBlack ? BLACK : WHITE);
    }
}

void Chess::drawFrame()
{
    // 直接调用基类的 drawFrame 来渲染棋盘
    Game::drawFrame();

    // 只保留游戏结束弹窗的渲染
    if (_gameStatus.showGameEndPopup)
    {
        renderGameStatus();
    }
}

void Chess::renderGameStatus()
{
    // 更新弹窗计时器
    if (_gameStatus.popupTimer > 0)
    {
        _gameStatus.popupTimer -= ImGui::GetIO().DeltaTime;
        if (_gameStatus.popupTimer <= 0)
        {
            _gameStatus.showCheckPopup = false;
            _gameStatus.showCapturePopup = false;
        }
    }

    // 游戏结束弹窗
    if (_gameStatus.showGameEndPopup)
    {
        // 确保弹窗总是打开
        if (!ImGui::IsPopupOpen("Game Over"))
        {
            ImGui::OpenPopup("Game Over");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_Modal;

        if (ImGui::BeginPopupModal("Game Over", nullptr, flags))
        {
            // 显示胜利信息
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.843f, 0.0f, 1.0f)); // 金色文字
            ImGui::SetWindowFontScale(1.5f);
            ImGui::TextWrapped("%s", _gameStatus.statusMessage.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 显示游戏统计信息
            ImGui::Text("Game Statistics:");
            ImGui::Text("Total Moves: %d", _gameOptions.currentTurnNo);
            // 可以添加更多统计信息，如游戏时长、吃子数等

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 按钮居中
            float buttonWidth = 120.0f;
            float windowWidth = ImGui::GetWindowSize().x;
            float buttonsStartX = (windowWidth - buttonWidth * 2.0f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

            ImGui::SetCursorPosX(buttonsStartX);

            if (ImGui::Button("New Game", ImVec2(buttonWidth, 0)))
            {
                // 重置游戏
                _gameStatus.showGameEndPopup = false;
                _isWhiteTurn = true;
                _castlingRights = CastlingRights(); // 重置王车易位权限
                setupInitialPosition();             // 重新设置棋盘
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Exit", ImVec2(buttonWidth, 0)))
            {
                _gameStatus.showGameEndPopup = false;
                ImGui::CloseCurrentPopup();
                // 可以添加退出游戏的逻辑
            }

            ImGui::EndPopup();
        }
    }

    // 将军提示
    if (_gameStatus.showCheckPopup)
    {
        ImVec2 pos = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y - 100), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.8f);
        if (ImGui::Begin("Check", nullptr,
                         ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoMove))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("Check!");
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }

    // 吃子提示
    if (_gameStatus.showCapturePopup)
    {
        ImVec2 pos = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + 100), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.8f);
        if (ImGui::Begin("Capture", nullptr,
                         ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoMove))
        {
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "%s", _gameStatus.statusMessage.c_str());
        }
        ImGui::End();
    }
}
