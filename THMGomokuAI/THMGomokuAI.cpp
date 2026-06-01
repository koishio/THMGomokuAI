// THMGomokuAI.cpp: 五子棋人机对战主程序（支持重新开局、选择先手）
#include "ZhiZhangAIService.h"

#include <iostream>
#include <vector>
#include <limits>
#include <concepts>
#include <type_traits>
#include <memory>
#include <future>
#include <chrono>
#include <thread>

// 辅助函数：打印棋盘
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
concept Streamable = requires(std::istream & is, T & v) {
    is >> v;
};

template <Streamable T, typename Pred>
    requires std::predicate<Pred&, const T&>
T getInput(const std::string& message, Pred&& condition) {
    T input;
    while (true) {
        std::cout << message;
        std::cin >> input;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << " ! 输入格式错误，请重新输入！" << std::endl;
            continue;
        }
        if (condition(input)) {
            break;
        }
        std::cout << " ! 输入值不满足条件，请重新输入！" << std::endl;
    }
    return input;
}

// 询问是否重新开始
bool askReplay() {
    char c = getInput<char>("是否重新开始并重新配置参数？(Y/N): ",
        [](char ch) { return ch == 'Y' || ch == 'y' || ch == 'N' || ch == 'n'; });
    return c == 'Y' || c == 'y';
}

// 选择谁先走
bool chooseFirstPlayer() {
    int choice = getInput<int>("选择先手 (1: 玩家先走, 2: AI先走): ",
        [](int v) { return v == 1 || v == 2; });
    return choice == 2;  // true = AI先手, false = 玩家先手
}

int main() {
    bool replay = true;
    while (replay) {
        // ----- 配置 AI 难度 -----
        int depth = 0;
        int maxNodes = 0;
        bool debug = false;
        int vcx = 0;
        int vcxDepth = 0;
        std::unique_ptr<AIConfig> config = nullptr;

        std::cout << "----- [ Touhou Little Maid 五子棋 AI 对战 ] -----\n\n";
        std::cout << "  * 五子棋 AI 原项目地址: https://github.com/TartaricAcid/TouhouLittleMaid\n\n";

        char choice = getInput<char>("是否使用默认难度配置 (Y/N): ",
            [](char c) {
                return c == 'Y' || c == 'y' || c == 'N' || c == 'n';
            });

        if (choice != 'N' && choice != 'n') {
            std::cout << " * 使用默认难度配置 (深度=6, 最大节点=10, 算杀关闭).\n";
            config = std::make_unique<AIConfig>(6, 10, false, 0, 6);
        }
        else {
            char debugChar = getInput<char>("是否开启调试模式 (Y/N): ",
                [](char c) { return c == 'Y' || c == 'y' || c == 'N' || c == 'n'; });
            debug = debugChar == 'Y' || debugChar == 'y';

            depth = getInput<int>("输入搜索深度 (正整数): ",
                [](int v) { return v > 0; });

            maxNodes = getInput<int>("输入最大候选点数目 (正整数): ",
                [](int v) { return v > 0; });

            vcx = getInput<int>("输入算杀模式 0:关闭 1:VCF 2:VCT: ",
                [](int v) { return v >= 0 && v <= 2; });

            if (vcx != 0) {
                vcxDepth = getInput<int>("输入算杀深度 (正整数): ",
                    [](int v) { return v > 0; });
            }
            config = std::make_unique<AIConfig>(depth, maxNodes, debug, vcx, vcxDepth);
        }

        // 选择先手
        bool aiFirst = chooseFirstPlayer();
        std::cout << (aiFirst ? " * AI 先手（黑棋）" : " * 玩家先手（黑棋）") << std::endl;

        // 创建 AI 服务
        ZhiZhangAIService ai(*config);

        const int BOARD_SIZE = 15;
        std::vector<std::vector<int>> chessData(BOARD_SIZE, std::vector<int>(BOARD_SIZE, 0));

        int playerType = 1;   // 玩家黑棋
        int aiType = 2;       // AI 白棋
        bool playerTurn = !aiFirst;  // AI先手则 playerTurn = false
        bool gameOver = false;
        Point lastMove;  // 记录上一步，初始为无效点

        // 如果 AI 先手，让它走第一步（天元）
        if (aiFirst) {
            // 传入一个虚拟的上一步（玩家类型），AI 会根据规则首子天元
            Point dummyLastMove(-1, -1, playerType);
            std::future<Point> future = std::async(std::launch::async, &ZhiZhangAIService::getPoint, &ai, std::cref(chessData), dummyLastMove);
            const char spin[] = "|/-\\";
            int spin_idx = 0;
            while (future.wait_for(std::chrono::milliseconds(200)) != std::future_status::ready) {
                std::cout << "\r * AI 思考中 " << spin[spin_idx % 4] << std::flush;
                spin_idx = (spin_idx + 1) % 4;
            }
            std::cout << "\r" << std::string(20, ' ') << "\r";
            Point aiMove = future.get();
            if (aiMove.x < 0 || aiMove.x >= BOARD_SIZE || aiMove.y < 0 || aiMove.y >= BOARD_SIZE ||
                chessData[aiMove.x][aiMove.y] != 0) {
                std::cout << "AI 返回无效落子，游戏终止。" << std::endl;
                break;
            }
            chessData[aiMove.x][aiMove.y] = aiMove.type;
            lastMove = aiMove;
            std::cout << "AI 落子: (" << aiMove.x << "," << aiMove.y << ")" << std::endl;
            printBoard(chessData);

            // 检查 AI 是否直接胜利（几乎不可能，但保留）
            Statue status = ai.getStatue(chessData, aiMove);
            if (status == Statue::WIN) {
                std::cout << "AI 胜利！你输了。" << std::endl;
                gameOver = true;
            }
            else if (status == Statue::DRAW) {
                std::cout << "平局！" << std::endl;
                gameOver = true;
            }
            // 切换回合
            playerTurn = true;  // 轮到玩家
        }

        // ----- 主游戏循环 -----
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
                    std::cout << " ! 坐标无效，请重新输入！" << std::endl;
                    continue;
                }
                if (chessData[x][y] != 0) {
                    std::cout << " ! 此处已有棋子，请重新选择！" << std::endl;
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
                std::future<Point> future = std::async(std::launch::async, &ZhiZhangAIService::getPoint, &ai, std::cref(chessData), lastMove);
                const char spin[] = "|/-\\";
                int spin_idx = 0;
                while (future.wait_for(std::chrono::milliseconds(200)) != std::future_status::ready) {
                    std::cout << "\r * AI 思考中 " << spin[spin_idx % 4] << std::flush;
                    spin_idx = (spin_idx + 1) % 4;
                }
                std::cout << "\r" << std::string(20, ' ') << "\r";
                Point aiMove = future.get();
                if (aiMove.x < 0 || aiMove.x >= BOARD_SIZE || aiMove.y < 0 || aiMove.y >= BOARD_SIZE ||
                    chessData[aiMove.x][aiMove.y] != 0) {
                    std::cout << " ! AI 返回异常落子，游戏结束。" << std::endl;
                    gameOver = true;
                    break;
                }
                chessData[aiMove.x][aiMove.y] = aiMove.type;
                lastMove = aiMove;

                std::cout << "AI 落子: (" << aiMove.x << "," << aiMove.y << ")" << std::endl;
                printBoard(chessData);

                Statue status = ai.getStatue(chessData, aiMove);
                if (status == Statue::WIN) {
                    std::cout << "AI 胜利！你输了。" << std::endl;
                    gameOver = true;
                    break;
                }
                else if (status == Statue::DRAW) {
                    std::cout << "平局！" << std::endl;
                    gameOver = true;
                    break;
                }

                playerTurn = true;
            }
        }

        std::cout << "游戏结束。" << std::endl;

        // 询问是否重新开始，并清理输入缓冲区中可能遗留的换行符
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        replay = askReplay();
    }

    return 0;
}