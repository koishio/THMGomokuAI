/*
 * This file is a C++ adaptation of the Gomoku AI from the Touhou Little Maid project.
 * Original work: Copyright (c) 2019-2025 tartaric_acid, for the code part
 * Modifications: Copyright (c) 2026 koishio
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the root of this project for details.
 */

// ZhiZhangAIService.h
// commit: db8bd02

#ifndef ZHI_ZHANG_AI_SERVICE_H
#define ZHI_ZHANG_AI_SERVICE_H

#include <vector>
#include <stack>
#include <unordered_map>
#include <random>
#include <string>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <iostream>

// 简单的 Point 结构体
struct Point {
    int x, y, type;
    int score;  // 用于评估分数

    Point(int x = 0, int y = 0, int type = 0, int score = 0)
        : x(x), y(y), type(type), score(score) {}

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y && type == other.type;
    }

    // 用于打印
    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        os << "(" << p.x << "," << p.y << "," << p.type << ")";
        return os;
    }
};

// 游戏状态枚举
enum class Statue {
    WIN,         // 胜利
    DRAW,        // 平局
    IN_PROGRESS  // 进行中
};

// AI 配置
struct AIConfig {
    int depth;          // 搜索深度
    int maxNodes;       // 最大节点数
    bool debug;         // 调试模式
    int vcx;            // 算杀模式 0:关闭 1:VCF 2:VCT
    int vcxDepth;       // 算杀深度

    AIConfig(int depth = 6, int maxNodes = 10, bool debug = false, int vcx = 0, int vcxDepth = 6)
        : depth(depth), maxNodes(maxNodes), debug(debug), vcx(vcx), vcxDepth(vcxDepth) {}
};

// AI 服务接口
class AIService {
public:
    virtual ~AIService() = default;
    virtual Point getPoint(const std::vector<std::vector<int>>& chessData, const Point& point) = 0;
    virtual Statue getStatue(const std::vector<std::vector<int>>& chessData, const Point& point) = 0;
};

// 智障AI服务实现
class ZhiZhangAIService : public AIService {
public:
    ZhiZhangAIService();
    explicit ZhiZhangAIService(const AIConfig& aiConfig);

    Point getPoint(const std::vector<std::vector<int>>& chessData, const Point& point) override;
    Statue getStatue(const std::vector<std::vector<int>>& chessData, const Point& point) override;

private:
    // 棋型枚举（保持 private）
    enum class ChessModel { LIANWU, HUOSI, HUOSAN, CHONGSI, HUOER, HUOYI, MIANSAN, MIANER, MIANYI };

    // 棋型信息结构
    struct ChessModelInfo {
        int score;
        std::vector<std::string> values;
    };

    // 静态棋型数据表（声明）
    static const std::vector<std::pair<ChessModel, ChessModelInfo>> CHESS_MODELS;

    // 风险分数枚举
    enum class RiskScore {
        HIGH_RISK = 800000,
        MEDIUM_RISK = 500000,
        LOW_RISK = 100000
    };

    // 统计结构
    struct Statistics {
        int depth = 0;
        Point point;
        int score = 0;
        double minimaxTime = 0.0;
        int nodes = 0;
        int cuts = 0;
        int vcxDepth = 0;
        int vcx = 0;          // 0:未命中 1:VCF 2:VCT
        double vcxTime = 0.0;
        int caches = 0;
        int cacheHits = 0;

        void incrNodes() { nodes++; }
        void incrCuts() { cuts++; }
        void incrCacheHits() { cacheHits++; }
    };

    // 局面缓存
    struct SituationCache {
        Point point;
        int score;
        int depth;

        SituationCache(int score = 0, int depth = 0) : score(score), depth(depth) {}
        SituationCache(const Point& p, int d) : point(p), score(0), depth(d) {}
    };

    // 静态成员：Zobrist 随机数表
    static std::vector<std::vector<int64_t>> BLACK_ZOBRIST;
    static std::vector<std::vector<int64_t>> WHITE_ZOBRIST;
    // static std::unordered_map<std::string, int> SCORE;  // 棋型分数表
    static std::mt19937_64 RNG;                         // 随机数生成器

    // 初始化静态成员
    static void initStaticData();

    // 辅助方法
    void initChessData(const std::vector<std::vector<int>>& chessData);
    void putChess(const Point& point);
    void revokeChess(const Point& point);
    int64_t calculateHashCode(const Point& point);
    Point getBestPoint(const Point& point);
    int minimax(int type, int depth);
    int minimax(int type, int depth, int alpha, int beta);
    std::vector<Point> getHeuristicPoints(int type);
    Point deepeningMinimax(int depth, int maxDepth);
    Point deepeningVcx(bool isAi, int maxDepth, bool isVcf);
    Point deepening(int depth, int maxDepth, bool isVcf);
    Point vcx(int type, int depth, bool isVcf);
    std::vector<Point> getVcxPoints(int type, bool isVcf);
    bool checkHighPriorityPoint(const Point& point);
    Point getBestPoint(const std::vector<Point>& pointList);
    Point getRandomBestPoint(const std::vector<Point>& pointList);
    std::vector<Point> randomPoint(int type, int num);
    int evaluate(const Point& point);
    int evaluateAll();
    bool checkSituation(const Point& point, const std::vector<ChessModel>& chessModels);
    static bool checkSituation(const std::string& situation, ChessModel chessModel);
    static ChessModel getChessModel(const std::string& situation);
    static int getScore(const std::string& situation);
    std::string getSituation(const Point& point, int direction);
    void appendChess(std::string& sb, const Point& point, int direction, int offset);
    int relativePoint(const Point& point, int direction, int offset) const;

    // 成员变量
    std::vector<std::vector<int>> chessData;   // 棋盘数据
    int rows = 0, cols = 0;                    // 棋盘行列数
    int ai = 0;                                // AI 棋子类型 (1黑 2白)
    float attack = 0.0f;                      // 进攻系数
    Point bestPoint;                          // 最佳落子点
    int rounds = 0;                           // 当前回合数
    Statistics statistics;                    // 统计信息
    std::stack<Point> pathStack;              // 算杀路径栈
    std::stack<Point> bestPathStack;          // 最佳算杀路径
    int64_t hashcode = 0;                     // 当前局面的 Zobrist 哈希
    std::unordered_map<int64_t, SituationCache> situationCacheMap; // 局面缓存

    AIConfig aiConfig;                        // AI 配置

    static const int INF = 999999999;    // 无穷大
    static const int BOARD_SIZE = 15;         // 棋盘最大尺寸
};

#endif // ZHIZHANGAISERVICE_H
