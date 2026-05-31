// THMGomokuAI.cpp: 五子棋人机对战主程序
#include "ZhiZhangAIService.h"
#include <iostream>
#include <vector>
#include <limits>
#include <functional>

// 辅助函数：打印棋盘（可选，用于调试）
void printBoard(const std::vector<std::vector<int>>& board) {
    std::cout << "\n   ";
    for (int i = 0; i < board.size(); ++i)
        std::cout << i % 10 << " ";
    std::cout << "\n";
    for (int y = 0; y < board[0].size(); ++y) {
        std::cout << y % 10 << "  ";
        for (int x = 0; x < board.size(); ++x) {
            char ch = ' ';
            if (board[x][y] == 1) ch = '@';
            else if (board[x][y] == 2) ch = 'O';
            else ch = '.';
            std::cout << ch << ' ';
        }
        std::cout << '\n';
    }
    std::cout << std::endl;
}

template <typename T>
T getInput(const std::string& message, std::function<bool(const T&)> condition) {
    T input;
    while (true) {
        std::cout << message;
        std::cin >> input;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入格式错误，请重新输入！" << std::endl;
            continue;
        }
        if (condition(input)) {
            break;
        }
        std::cout << "输入值不满足条件，请重新输入！" << std::endl;
    }
    return input;
}

int main() {
    // 强 AI 配置
    int depth = 0;
    int maxNodes = 0;
    bool debug = false;
    int vcx = 0;
    int vcxDepth = 0;
    std::unique_ptr<AIConfig> config = nullptr;

    std::cout << "----- [ Touhou Little Maid 五子棋 AI 对战 ] -----\n\n";
    std::cout << "  * 原项目地址: https://github.com/TartaricAcid/TouhouLittleMaid\n\n";

    char choice = getInput<char>("是否使用默认难度配置 (Y/N): ",
        [](char c) {
            return c == 'Y' || c == 'y' || c == 'N' || c == 'n';
        });

    if (choice != 'N' && choice != 'n') {
        std::cout << "使用默认难度配置.\n";
        config = std::make_unique<AIConfig>(6, 10, false, 0, 6);
    } else {
        char debug_c = getInput<char>("是否开启调试模式 (Y/N): ",
            [](char c) {
                return c == 'Y' || c == 'y' || c == 'N' || c == 'n';
            });
        debug = debug_c == 'Y' || debug_c == 'y';
        depth = getInput<int>("输入搜索深度 (默认为6): ",
            [](int v) {
                return v > 0;
            });
        maxNodes = getInput<int>("输入最大候选点数目 (默认为10): ",
            [](int v) {
                return v > 0;
            });
        vcx = getInput<int>("输入算杀模式 0:关闭 1:VCF 2:VCT (默认为0): ",
            [](int v) {
                return v >= 0 && v <= 2;
            });
        if (vcx != 0) {
            vcxDepth = getInput<int>("输入算杀深度: ",
                [](int v) {
                    return v > 0;
                });
        }
        config = std::make_unique<AIConfig>(depth, maxNodes, debug, vcx, vcxDepth);
    }

    ZhiZhangAIService ai(*config);

    const int BOARD_SIZE = 15;
    std::vector<std::vector<int>> chessData(BOARD_SIZE, std::vector<int>(BOARD_SIZE, 0));

    int playerType = 1;   // 玩家黑棋先手
    int aiType = 2;
    bool playerTurn = true;
    bool gameOver = false;
    Point lastMove;

    // 如果 AI 先走，需要先执行一步 AI 落子（取消下面注释）
    /*
    if (!playerTurn) {
        Point aiMove = ai.getPoint(chessData, Point(0, 0, playerType)); // 传入无效点
        chessData[aiMove.x][aiMove.y] = aiMove.type;
        lastMove = aiMove;
        playerTurn = true;   // 切换为玩家走
        std::cout << "女仆 落子: (" << aiMove.x << "," << aiMove.y << ")" << std::endl;
        printBoard(chessData);
        // 检查 AI 是否胜利（略，后面循环会统一判断）
    }
    */

    // 主游戏循环
    while (!gameOver) {
        if (playerTurn) {
            // ----- 玩家回合 -----
            int x, y;
            std::cout << "你的回合（输入坐标 x y，范围 0~14）: ";
            std::cin >> x >> y;

            // 输入合法性检查
            if (std::cin.fail() || x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "坐标无效，请重新输入！" << std::endl;
                continue;
            }
            if (chessData[x][y] != 0) {
                std::cout << "此处已有棋子，请重新选择！" << std::endl;
                continue;
            }

            // 落子
            Point playerMove(x, y, playerType);
            chessData[x][y] = playerType;
            lastMove = playerMove;

            // 显示棋盘
            printBoard(chessData);

            // 判断胜负/平局
            Statue status = ai.getStatue(chessData, playerMove);
            if (status == Statue::WIN) {
                std::cout << "恭喜！你赢了！" << std::endl;
                gameOver = true;
                break;
            }
            else if (status == Statue::DRAW) {
                std::cout << "平局！" << std::endl;
                gameOver = true;
                break;
            }

            // 切换回合
            playerTurn = false;
        }
        else {
            // ----- AI 回合 -----
            std::cout << "女仆 思考中..." << std::endl;
            // 传入对手的上一步（玩家刚刚下的棋子），AI 会基于此更新内部状态
            Point aiMove = ai.getPoint(chessData, lastMove);
            // 确保 AI 返回有效落子
            if (aiMove.x < 0 || aiMove.x >= BOARD_SIZE || aiMove.y < 0 || aiMove.y >= BOARD_SIZE ||
                chessData[aiMove.x][aiMove.y] != 0) {
                // 理论上 AI 不会返回无效点，这里做防御
                std::cout << "女仆 返回异常落子，游戏结束." << std::endl;
                gameOver = true;
                break;
            }
            chessData[aiMove.x][aiMove.y] = aiMove.type;
            lastMove = aiMove;

            std::cout << "女仆 落子: (" << aiMove.x << "," << aiMove.y << ")" << std::endl;
            printBoard(chessData);

            // 判断胜负/平局
            Statue status = ai.getStatue(chessData, aiMove);
            if (status == Statue::WIN) {
                std::cout << "女仆 胜利！你输了." << std::endl;
                gameOver = true;
                break;
            }
            else if (status == Statue::DRAW) {
                std::cout << "平局！" << std::endl;
                gameOver = true;
                break;
            }

            // 切换回合
            playerTurn = true;
        }
    }

    std::cout << "游戏结束. 按 Enter 退出." << std::endl;

    std::cin.get();
    std::cin.get();

    return 0;
}