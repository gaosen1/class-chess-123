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

    int pieceType = bit.gameTag() & 127;

    // 检查王车易位
    if (pieceType == KING && abs(dstCol - srcCol) == 2)
    {
        bool isKingSide = dstCol > srcCol;
        if (canCastle(isKingSide, isBlack))
        {
            // 设置王车易位状态
            _castlingState.inProgress = true;
            _castlingState.isKingSide = isKingSide;
            _castlingState.rookFromCol = isKingSide ? 7 : 0;
            _castlingState.rookToCol = isKingSide ? 5 : 3;
            printf("\n=== Castling Initiated ===\n");
            printf("%s side castling for %s\n",
                   isKingSide ? "King" : "Queen",
                   isBlack ? "Black" : "White");
            return true;
        }
        return false;
    }

    // 检查吃过路兵
    if (pieceType == PAWN)
    {
        // 检查是否是吃过路兵的情况
        if (_enPassantState.available)
        {
            int expectedRow = isBlack ? 3 : 4; // 吃过路兵的兵应该在的行

            // 检查移动的兵是否正确的行
            if (srcRow == expectedRow)
            {
                // 检查是否是斜向移动到过路兵的位置
                if (abs(srcCol - dstCol) == 1 &&
                    dstCol == _enPassantState.pawnCol &&
                    dstRow == (isBlack ? 2 : 5))
                {
                    // 确认目标位置是过路兵的位置
                    Bit *targetPawn = _grid[srcRow][dstCol].bit(); // 注意：使用srcRow而不是dstRow
                    if (targetPawn &&
                        (targetPawn->gameTag() & 127) == PAWN &&
                        ((targetPawn->gameTag() & 128) != 0) != isBlack)
                    {
                        printf("\n=== En Passant Available ===\n");
                        printf("%s pawn at [%d,%d] can capture en passant at [%d,%d]\n",
                               isBlack ? "Black" : "White",
                               srcRow, srcCol,
                               dstRow, dstCol);
                        return true;
                    }
                }
            }
        }

        // 记录双步移动（为下一回合的吃过路兵做准备）
        if (abs(dstRow - srcRow) == 2)
        {
            _enPassantState.pawnRow = srcRow + (dstRow - srcRow) / 2; // 记录中间行
            _enPassantState.pawnCol = dstCol;
            _enPassantState.available = true;
            printf("\n=== En Passant Target Created ===\n");
            printf("Pawn at [%d,%d] can be captured en passant next turn\n",
                   dstRow, dstCol);
        }
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

    // 检查是否是吃过路兵的情况
    if (pieceType == PAWN)
    {
        // 检查是否是吃过路兵的移动
        if (_enPassantState.available &&
            abs(srcCol - dstCol) == 1 &&         // 斜向移动
            dstCol == _enPassantState.pawnCol && // 目标列是过路兵的列
            ((isBlack && srcRow == 3 && dstRow == 2) ||
             (!isBlack && srcRow == 4 && dstRow == 5))) // 正确的行
        {
            // 移除被吃掉的过路兵（在原来的行）
            Bit *capturedPawn = _grid[srcRow][dstCol].bit();
            if (capturedPawn)
            {
                printf("\n=== En Passant Capture ===\n");
                printf("%s pawn at [%d,%d] captured en passant at [%d,%d]\n",
                       isBlack ? "Black" : "White",
                       srcRow, srcCol,
                       srcRow, dstCol);

                // 先移除被吃的兵
                _grid[srcRow][dstCol].setBit(nullptr);
                delete capturedPawn;
            }
        }

        // 检查是否是双步移动（为下一回合的吃过路兵做准备）
        if (abs(dstRow - srcRow) == 2)
        {
            _enPassantState.pawnRow = srcRow + (dstRow - srcRow) / 2;
            _enPassantState.pawnCol = dstCol;
            _enPassantState.available = true;
            printf("\n=== En Passant Target Created ===\n");
            printf("Pawn at [%d,%d] can be captured en passant next turn\n",
                   dstRow, dstCol);
        }
        else
        {
            // 如果不是双步移动，重置吃过路兵状态
            _enPassantState.available = false;
            _enPassantState.pawnRow = -1;
            _enPassantState.pawnCol = -1;
        }
    }

    // 执行正常的移动
    dst.setBit(&bit);
    bit.setPosition(dst.getPosition());

    // 然后处理王车易位
    if (_castlingState.inProgress)
    {
        int rookRow = isBlack ? 7 : 0;

        // 获取车的位置
        Bit *rook = _grid[rookRow][_castlingState.rookFromCol].bit();
        if (rook)
        {
            // 计算车的新位置
            ImVec2 newRookPos = _grid[rookRow][_castlingState.rookToCol].getPosition();

            // 移动车
            _grid[rookRow][_castlingState.rookToCol].setBit(rook);
            _grid[rookRow][_castlingState.rookFromCol].setBit(nullptr);

            // 添加动画效果
            rook->moveTo(newRookPos);

            printf("Rook moved from [%d,%d] to [%d,%d]\n",
                   rookRow, _castlingState.rookFromCol,
                   rookRow, _castlingState.rookToCol);
        }

        // 重置王车易位状态
        _castlingState.inProgress = false;

        // 更新王车易位权限
        if (isBlack)
        {
            _castlingRights.blackKingMoved = true;
            _castlingRights.blackRookKingside = true;
            _castlingRights.blackRookQueenside = true;
        }
        else
        {
            _castlingRights.whiteKingMoved = true;
            _castlingRights.whiteRookKingside = true;
            _castlingRights.whiteRookQueenside = true;
        }

        printf("=== Castling Completed ===\n");
    }

    // 重置吃过路兵状态（在回合结束时）
    if (!_enPassantState.available)
    {
        _enPassantState.pawnRow = -1;
        _enPassantState.pawnCol = -1;
    }

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
    if (!_isWhiteTurn && _gameOptions.AIPlayer)
    {
        // 检查是否有棋子正在移动
        bool piecesMoving = false;
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                Bit *piece = _grid[row][col].bit();
                if (piece && piece->getMoving())
                {
                    piecesMoving = true;
                    break;
                }
            }
            if (piecesMoving)
                break;
        }

        // 如果有棋子在移动，等待移动完成
        if (piecesMoving)
        {
            return;
        }

        static bool startedThinking = false;
        static auto startTime = std::chrono::steady_clock::now();

        // 开始思考
        if (!startedThinking)
        {
            printf("\n=== AI Thinking ===\n");
            startTime = std::chrono::steady_clock::now();
            startedThinking = true;
            return;
        }

        // 思考延时（改为0.8秒）
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

        if (elapsedTime < 800) // 缩短到0.8秒
        {
            return; // 继续"思考"
        }

        // 使用 Minimax 算法找到最佳移动
        auto [from, to] = findBestMove(true, 3);
        int fromRow = from.first, fromCol = from.second;
        int toRow = to.first, toCol = to.second;

        // 执行 AI 的移动
        if (fromRow != -1)
        {
            Bit *piece = _grid[fromRow][fromCol].bit();
            if (piece)
            {
                BitHolder &src = _grid[fromRow][fromCol];
                BitHolder &dst = _grid[toRow][toCol];

                printf("AI Move: From [%d,%d] To [%d,%d]\n", fromRow, fromCol, toRow, toCol);

                // 执行移动
                bitMovedFromTo(*piece, src, dst);
            }
        }
        else
        {
            printf("AI couldn't find a valid move!\n");
        }

        // 重置思考状态
        startedThinking = false;
    }
}

int Chess::evaluatePosition() const
{
    int score = 0;
    // 棋子基础价值
    const int pieceValues[] = {0, 100, 320, 330, 500, 900, 20000}; // EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING

    // 位置价值表（鼓励棋子控制中心和重要位置）
    const int pawnPositionBonus[8][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {50, 50, 50, 50, 50, 50, 50, 50},
        {10, 10, 20, 30, 30, 20, 10, 10},
        {5, 5, 10, 25, 25, 10, 5, 5},
        {0, 0, 0, 20, 20, 0, 0, 0},
        {5, -5, -10, 0, 0, -10, -5, 5},
        {5, 10, 10, -20, -20, 10, 10, 5},
        {0, 0, 0, 0, 0, 0, 0, 0}};

    const int knightPositionBonus[8][8] = {
        {-50, -40, -30, -30, -30, -30, -40, -50},
        {-40, -20, 0, 0, 0, 0, -20, -40},
        {-30, 0, 10, 15, 15, 10, 0, -30},
        {-30, 5, 15, 20, 20, 15, 5, -30},
        {-30, 0, 15, 20, 20, 15, 0, -30},
        {-30, 5, 10, 15, 15, 10, 5, -30},
        {-40, -20, 0, 5, 5, 0, -20, -40},
        {-50, -40, -30, -30, -30, -30, -40, -50}};

    // 遍历棋盘
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Bit *piece = _grid[row][col].bit();
            if (!piece)
                continue;

            int pieceType = piece->gameTag() & 127;
            bool isBlack = (piece->gameTag() & 128) != 0;
            int value = pieceValues[pieceType];

            // 基础分值
            score += isBlack ? -value : value;

            // 位置加分
            int positionBonus = 0;
            switch (pieceType)
            {
            case PAWN:
                positionBonus = pawnPositionBonus[isBlack ? (7 - row) : row][col];
                break;
            case KNIGHT:
                positionBonus = knightPositionBonus[isBlack ? (7 - row) : row][col];
                break;
            default:
                // 其他棋子的中心控制加分
                positionBonus = (4 - abs(col - 3.5)) * 10 + (4 - abs(row - 3.5)) * 10;
            }
            score += isBlack ? -positionBonus : positionBonus;

            // 机动性加分（可移动的格子数）
            auto moves = getBasicLegalMoves(*piece, row, col, true);
            int mobilityBonus = moves.size() * 10;
            score += isBlack ? -mobilityBonus : mobilityBonus;

            // 控制中心的加分
            if ((row == 3 || row == 4) && (col == 3 || col == 4))
            {
                score += isBlack ? -30 : 30;
            }

            // 保护王的加分
            auto [kingRow, kingCol] = getKingPosition(isBlack);
            if (abs(row - kingRow) <= 2 && abs(col - kingCol) <= 2)
            {
                score += isBlack ? -20 : 20;
            }
        }
    }

    // 将军状态的额外评估
    if (isInCheck(!_isWhiteTurn))
    {
        score += _isWhiteTurn ? 100 : -100;
    }

    return score;
}

std::pair<std::pair<int, int>, std::pair<int, int>> Chess::findBestMove(bool isBlack, int depth)
{
    int bestScore = isBlack ? INT_MAX : INT_MIN;
    std::pair<int, int> bestFrom = {-1, -1};
    std::pair<int, int> bestTo = {-1, -1};

    // 遍历所有可能的移动
    for (int fromRow = 0; fromRow < 8; fromRow++)
    {
        for (int fromCol = 0; fromCol < 8; fromCol++)
        {
            Bit *piece = _grid[fromRow][fromCol].bit();
            if (!piece || ((piece->gameTag() & 128) != 0) != isBlack)
                continue;

            auto moves = getLegalMoves(*piece, fromRow, fromCol);
            for (const auto &move : moves)
            {
                int toRow = move.first;
                int toCol = move.second;

                // 保存当前状态
                Bit *capturedPiece = _grid[toRow][toCol].bit();

                // 尝试移动
                makeMove(fromRow, fromCol, toRow, toCol);

                // 计算这个移动的分数
                int score = minimax(depth - 1, !isBlack, INT_MIN, INT_MAX);

                // 撤销移动
                undoMove(fromRow, fromCol, toRow, toCol, capturedPiece);

                // 更新最佳移动
                if ((isBlack && score < bestScore) || (!isBlack && score > bestScore))
                {
                    bestScore = score;
                    bestFrom = {fromRow, fromCol};
                    bestTo = {toRow, toCol};
                }
            }
        }
    }
    return {bestFrom, bestTo};
}

int Chess::minimax(int depth, bool isMaximizing, int alpha, int beta)
{
    if (depth == 0)
        return evaluatePosition();

    int bestScore = isMaximizing ? INT_MIN : INT_MAX;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Bit *piece = _grid[row][col].bit();
            if (!piece || ((piece->gameTag() & 128) != 0) == isMaximizing)
                continue;

            auto moves = getLegalMoves(*piece, row, col);
            for (const auto &move : moves)
            {
                Bit *capturedPiece = _grid[move.first][move.second].bit();

                makeMove(row, col, move.first, move.second);
                int score = minimax(depth - 1, !isMaximizing, alpha, beta);
                undoMove(row, col, move.first, move.second, capturedPiece);

                if (isMaximizing)
                {
                    bestScore = std::max(bestScore, score);
                    alpha = std::max(alpha, bestScore);
                }
                else
                {
                    bestScore = std::min(bestScore, score);
                    beta = std::min(beta, bestScore);
                }

                if (beta <= alpha)
                    break;
            }
        }
    }
    return bestScore;
}

void Chess::makeMove(int fromRow, int fromCol, int toRow, int toCol)
{
    Bit *piece = _grid[fromRow][fromCol].bit();
    _grid[toRow][toCol].setBit(piece);
    _grid[fromRow][fromCol].setBit(nullptr);
}

void Chess::undoMove(int fromRow, int fromCol, int toRow, int toCol, Bit *capturedPiece)
{
    Bit *piece = _grid[toRow][toCol].bit();
    _grid[fromRow][fromCol].setBit(piece);
    _grid[toRow][toCol].setBit(capturedPiece);
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

        // 吃子移动（包括普通吃子和吃过路兵）
        for (int dc : {-1, 1})
        {
            int newCol = srcCol + dc;
            int newRow = srcRow + direction;

            if (isValidPosition(newRow, newCol))
            {
                // 检查普通吃子
                Bit *targetPiece = _grid[newRow][newCol].bit();
                if (targetPiece && ((targetPiece->gameTag() & 128) != 0) != isBlack)
                {
                    moves.push_back({newRow, newCol});
                }

                // 检查吃过路兵
                if (_enPassantState.available)
                {
                    int expectedRow = isBlack ? 3 : 4; // 吃过路兵的兵应该在的行
                    if (srcRow == expectedRow)         // 确保兵在正确的行
                    {
                        // 检查左右两个方向
                        for (int dc : {-1, 1})
                        {
                            int newCol = srcCol + dc;
                            if (isValidPosition(srcRow, newCol) &&
                                newCol == _enPassantState.pawnCol) // 目标列是过路兵的列
                            {
                                // 添加吃过路兵的目标位置
                                moves.push_back({isBlack ? 2 : 5, newCol});
                                printf("En passant move available from [%d,%d] to [%d,%d]\n",
                                       srcRow, srcCol, isBlack ? 2 : 5, newCol);
                            }
                        }
                    }
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
            {
                continue; // 如果有己方棋子，跳过这个移动
            }

            // 检查是否会与对方的王相邻
            auto [enemyKingRow, enemyKingCol] = getKingPosition(!isBlack);
            if (abs(enemyKingRow - newRow) <= 1 && abs(enemyKingCol - newCol) <= 1)
            {
                continue; // 不允许两个王相邻
            }

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

            // 如果检查王的攻击，跳过王
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
    int row = isBlack ? 7 : 0;
    int kingCol = 4;
    int rookCol = kingSide ? 7 : 0;

    // 1. 检查王和车是否还在初始位置
    Bit *king = _grid[row][kingCol].bit();
    Bit *rook = _grid[row][rookCol].bit();
    if (!king || !rook ||
        (king->gameTag() & 127) != KING ||
        (rook->gameTag() & 127) != ROOK)
    {
        return false;
    }

    // 2. 检查王和车是否移动过（使用 _castlingRights）
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

    // 3. 检查中间格子是否为空
    int startCol = kingCol + (kingSide ? 1 : -1);
    int endCol = kingSide ? rookCol - 1 : rookCol + 1;
    int step = kingSide ? 1 : -1;
    for (int col = startCol; kingSide ? (col <= endCol) : (col >= endCol); col += step)
    {
        if (_grid[row][col].bit() != nullptr)
        {
            return false;
        }
    }

    // 4. 检查王是否正在被将军
    if (isInCheck(isBlack))
    {
        return false;
    }

    // 5. 检查王经过的格子是否被攻击
    for (int col = kingCol; kingSide ? (col <= kingCol + 2) : (col >= kingCol - 2); col += step)
    {
        if (isSquareUnderAttack(row, col, !isBlack))
        {
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
        _grid[row][col].setBit(nullptr); // 先解除关联
        delete piece;                    // 然后安全删除
        placePiece(row, col, QUEEN, isBlack ? BLACK : WHITE);
    }
}

void Chess::drawFrame()
{
    // 获取主视口
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    // 创建控制面板窗口
    ImGuiWindowFlags control_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoNav;

    // 设置控制面板位置
    ImVec2 control_pos = viewport->Pos;
    control_pos.x += 10;
    control_pos.y += 10;

    ImGui::SetNextWindowPos(control_pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - 20, 50));
    ImGui::SetNextWindowViewport(viewport->ID);

    // 开始控制面板窗口
    ImGui::Begin("##Controls", nullptr, control_flags);

    // 设置按钮样式
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.8f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.9f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.7f, 0.7f));

    // 创建重置按钮
    if (ImGui::Button("Reset Game", ImVec2(100, 30)))
    {
        printf("\n=== Game Reset ===\n");
        resetGame();
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // 创建玩家对战按钮
    if (ImGui::Button("Player vs Player", ImVec2(120, 30)))
    {
        printf("\n=== Starting Player vs Player Mode ===\n");
        resetGame();
        _gameOptions.AIPlayer = false;
        _gameOptions.AIvsAI = false;
    }

    ImGui::SameLine();

    // 创建人机对战按钮
    if (ImGui::Button("Player vs AI", ImVec2(100, 30)))
    {
        printf("\n=== Starting Player vs AI Mode ===\n");
        resetGame();
        _gameOptions.AIPlayer = true; // AI 控制黑方
        _gameOptions.AIvsAI = false;
        setAIPlayer(1); // 设置黑方为 AI
    }

    // 恢复按钮样式
    ImGui::PopStyleColor(3);

    // 添加当前回合和模式信息
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SameLine(windowWidth - 300);
    ImGui::Text("Mode: %s", _gameOptions.AIPlayer ? "Player vs AI" : "Player vs Player");
    ImGui::SameLine(windowWidth - 150);
    ImGui::Text("Turn: %s", _isWhiteTurn ? "White" : "Black");

    ImGui::End();

    // 渲染棋盘和棋子
    Game::drawFrame();

    // 如果是 AI 的回合，执行 AI 移动
    if (_gameOptions.AIPlayer && !_isWhiteTurn)
    {
        updateAI();
    }

    // 渲染游戏状态弹窗
    if (_gameStatus.showGameEndPopup)
    {
        renderGameStatus();
    }
}

// 添加一个辅助函数来处理重置游戏的逻辑
void Chess::resetGame()
{
    // 重置游戏状态
    _gameStatus.showGameEndPopup = false;
    _isWhiteTurn = true;
    _castlingRights = CastlingRights();
    setupInitialPosition();

    // 清除历史记录
    for (auto &turn : _turns)
    {
        delete turn;
    }
    _turns.clear();

    // 添加初始回合
    Turn *turn = Turn::initStartOfGame(this);
    _turns.push_back(turn);

    // 重置回合数
    _gameOptions.currentTurnNo = 0;
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
        // 确保弹窗是打开
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

bool Chess::gameHasAI()
{
    // 如果是 AI 模式且当前是黑方回合，返回 true
    return _gameOptions.AIPlayer && !_isWhiteTurn;
}
