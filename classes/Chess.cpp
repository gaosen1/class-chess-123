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
    printf("Moving piece from src to dst\n");

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

    printf("Move from [%d,%d] to [%d,%d]\n", srcRow, srcCol, dstRow, dstCol);

    // 更新王车易位权限
    updateCastlingRights(bit, srcRow, srcCol);

    // 检查是否是王车易位
    int pieceType = bit.gameTag() & 127;
    if (pieceType == KING && abs(dstCol - srcCol) == 2)
    {
        printf("Performing castling\n");
        bool isKingSide = dstCol > srcCol;
        int rookSrcCol = isKingSide ? 7 : 0;
        int rookDstCol = isKingSide ? dstCol - 1 : dstCol + 1;

        // 移动车
        Bit *rook = _grid[srcRow][rookSrcCol].bit();
        if (rook)
        {
            printf("Moving rook from [%d,%d] to [%d,%d]\n",
                   srcRow, rookSrcCol, srcRow, rookDstCol);
            _grid[srcRow][rookDstCol].setBit(rook);
            rook->setPosition(_grid[srcRow][rookDstCol].getPosition());
            _grid[srcRow][rookSrcCol].setBit(nullptr);
        }
    }

    // 检查是否吃子
    Bit *capturedPiece = dst.bit();
    if (capturedPiece)
    {
        _gameStatus.showCapturePopup = true;
        _gameStatus.popupTimer = _gameStatus.POPUP_DURATION;
        bool isBlack = (bit.gameTag() & 128) != 0;
        const char *pieceNames[] = {"", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
        int capturedType = capturedPiece->gameTag() & 127;
        _gameStatus.statusMessage = std::string(isBlack ? "Black" : "White") +
                                    " captured " +
                                    std::string(!isBlack ? "Black" : "White") +
                                    "'s " + pieceNames[capturedType];
    }

    printf("Completing move\n");
    Game::bitMovedFromTo(bit, src, dst);
    _isWhiteTurn = !_isWhiteTurn;
    printf("Turn changed to %s\n", _isWhiteTurn ? "White" : "Black");

    // 检查将军和将死状态
    bool opponentIsBlack = !_isWhiteTurn;
    if (isInCheck(opponentIsBlack))
    {
        if (isCheckmate(opponentIsBlack))
        {
            _gameStatus.showGameEndPopup = true;
            _gameStatus.statusMessage = std::string(_isWhiteTurn ? "Black" : "White") +
                                        " wins by checkmate!";
        }
        else
        {
            _gameStatus.showCheckPopup = true;
            _gameStatus.popupTimer = _gameStatus.POPUP_DURATION;
        }
    }

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
        // 后的��动 - 合象和车的动
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
                    break; // 无论是否可以吃子，都不能继续前进
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

            // 创建临时棋盘状态来检查移动的安全性
            ChessSquare tempGrid[8][8];
            for (int r = 0; r < 8; r++)
            {
                for (int c = 0; c < 8; c++)
                {
                    tempGrid[r][c] = _grid[r][c];
                }
            }

            // 在临时棋盘上模拟移动
            tempGrid[srcRow][srcCol].setBit(nullptr);
            tempGrid[newRow][newCol].setBit(nullptr);

            // 检查新位置是否安全
            bool isSafe = true;
            for (int r = 0; r < 8; r++)
            {
                for (int c = 0; c < 8; c++)
                {
                    Bit *attackingPiece = tempGrid[r][c].bit();
                    if (!attackingPiece)
                        continue;

                    // 如果是对方的棋子
                    if (((attackingPiece->gameTag() & 128) != 0) != isBlack)
                    {
                        // 获取攻击棋子的基本移动（忽略王的移动以避免递归）
                        if ((attackingPiece->gameTag() & 127) == KING)
                            continue;

                        auto attackingMoves = getBasicLegalMoves(*attackingPiece, r, c, true);
                        if (std::find(attackingMoves.begin(), attackingMoves.end(),
                                      std::make_pair(newRow, newCol)) != attackingMoves.end())
                        {
                            isSafe = false;
                            break;
                        }
                    }
                }
                if (!isSafe)
                    break;
            }

            if (isSafe)
            {
                moves.push_back({newRow, newCol});
            }
        }
        break;
    }
    }

    return moves;
}

bool Chess::isSquareUnderAttack(int row, int col, bool byBlack, bool checkKing) const
{
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Bit *piece = _grid[r][c].bit();
            if (!piece)
                continue;

            // 如果不检查王的攻击，跳过王
            if (!checkKing && (piece->gameTag() & 127) == KING)
                continue;

            if (((piece->gameTag() & 128) != 0) == byBlack)
            {
                // 只获取基本移动，避免递归
                auto moves = getBasicLegalMoves(*piece, r, c, true);
                if (std::find(moves.begin(), moves.end(),
                              std::make_pair(row, col)) != moves.end())
                {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<std::pair<int, int>> Chess::getLegalMoves(const Bit &piece, int srcRow, int srcCol) const
{
    printf("Getting legal moves for piece at [%d,%d]\n", srcRow, srcCol);
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
    // 检查王是否被攻击，但不考虑对方王的攻击
    return isSquareUnderAttack(kingRow, kingCol, !blackKing, false);
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
    // 如果不在将军状态，就不可能是将死
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

            // 只检查己方棋子
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
        ImGui::OpenPopup("Game Over");
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Game Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", _gameStatus.statusMessage.c_str());
            if (ImGui::Button("OK"))
            {
                _gameStatus.showGameEndPopup = false;
                ImGui::CloseCurrentPopup();
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
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Check!");
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
            ImGui::Text("%s", _gameStatus.statusMessage.c_str());
        }
        ImGui::End();
    }
}

void Chess::render()
{
    Game::render();     // 调用基类的 render
    renderGameStatus(); // 渲染游戏状态提示
}
