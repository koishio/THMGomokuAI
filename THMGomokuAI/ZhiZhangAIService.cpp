#include "ZhiZhangAIService.h"

// 静态成员定义
std::vector<std::vector<int64_t>> ZhiZhangAIService::BLACK_ZOBRIST;
std::vector<std::vector<int64_t>> ZhiZhangAIService::WHITE_ZOBRIST;
std::mt19937_64 ZhiZhangAIService::RNG(std::chrono::steady_clock::now().time_since_epoch().count());

// 定义静态成员
const std::vector<std::pair<ZhiZhangAIService::ChessModel, ZhiZhangAIService::ChessModelInfo>>
ZhiZhangAIService::CHESS_MODELS = {
{ChessModel::LIANWU, {10000000, {"11111"}}},
{ChessModel::HUOSI,  {1000000,  {"011110"}}},
{ChessModel::HUOSAN, {10000,    {"001110", "011100", "010110", "011010"}}},
{ChessModel::CHONGSI,{9000,     {"11110", "01111", "10111", "11011", "11101"}}},
{ChessModel::HUOER,  {100,      {"001100", "011000", "000110", "001010", "010100"}}},
{ChessModel::HUOYI,  {80,       {"010200", "002010", "020100", "001020", "201000", "000102", "000201"}}},
{ChessModel::MIANSAN,{30,       {"001112", "010112", "011012", "211100", "211010"}}},
{ChessModel::MIANER, {10,       {"011200", "001120", "002110", "021100", "110000", "000011", "000112", "211000"}}},
{ChessModel::MIANYI, {1,        {"001200", "002100", "000210", "000120", "210000", "000012"}}}
};

void ZhiZhangAIService::initStaticData() {
    // 初始化 Zobrist 随机表
    BLACK_ZOBRIST.resize(BOARD_SIZE, std::vector<int64_t>(BOARD_SIZE));
    WHITE_ZOBRIST.resize(BOARD_SIZE, std::vector<int64_t>(BOARD_SIZE));
    std::uniform_int_distribution<int64_t> dist;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            BLACK_ZOBRIST[i][j] = dist(RNG);
            WHITE_ZOBRIST[i][j] = dist(RNG);
        }
    }
}

// 构造函数
ZhiZhangAIService::ZhiZhangAIService() : ZhiZhangAIService(AIConfig(6, 10, false, 0, 6)) {
    static bool initialized = false;
    if (!initialized) {
        initStaticData();
        initialized = true;
    }
}

ZhiZhangAIService::ZhiZhangAIService(const AIConfig& config) : aiConfig(config) {
    static bool initialized = false;
    if (!initialized) {
        initStaticData();
        initialized = true;
    }
}

// 初始化棋盘数据
void ZhiZhangAIService::initChessData(const std::vector<std::vector<int>>& data) {
    rows = static_cast<int>(data.size());
    cols = static_cast<int>(data[0].size());
    chessData.assign(cols, std::vector<int>(rows, 0));
    int chessTotal = 0;
    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            int type = data[i][j];
            if (type != 0) {
                putChess(Point(i, j, type));
                chessTotal++;
            }
        }
    }
    rounds = chessTotal / 2 + 1;
}

// 落子并更新哈希
void ZhiZhangAIService::putChess(const Point& point) {
    chessData[point.x][point.y] = point.type;
    calculateHashCode(point);
}

// 撤销落子
void ZhiZhangAIService::revokeChess(const Point& point) {
    chessData[point.x][point.y] = 0;
    calculateHashCode(point);
}

// 计算 Zobrist 哈希
int64_t ZhiZhangAIService::calculateHashCode(const Point& point) {
    int x = point.x, y = point.y;
    hashcode ^= (point.type == 1) ? BLACK_ZOBRIST[x][y] : WHITE_ZOBRIST[x][y];
    return hashcode;
}

// 判断胜负/平局
Statue ZhiZhangAIService::getStatue(const std::vector<std::vector<int>>& chessData, const Point& point) {
    int rows = static_cast<int>(chessData[0].size());
    int cols = static_cast<int>(chessData.size());
    int x = point.x;
    int y = point.y;
    int type = point.type;

    // 横轴
    int k = 1;
    for (int i = 1; i < 5; ++i) {
        int preX = x - i;
        if (preX < 0) break;
        if (chessData[preX][y] != type) break;
        if (++k == 5) return Statue::WIN;
    }
    for (int i = 1; i < 5; ++i) {
        int nextX = x + i;
        if (nextX >= rows) break;
        if (chessData[nextX][y] != type) break;
        if (++k == 5) return Statue::WIN;
    }

    // 纵轴
    k = 1;
    for (int i = 1; i < 5; ++i) {
        int preY = y - i;
        if (preY < 0) break;
        if (chessData[x][preY] != type) break;
        if (++k == 5) return Statue::WIN;
    }
    for (int i = 1; i < 5; ++i) {
        int nextY = y + i;
        if (nextY >= cols) break;
        if (chessData[x][nextY] != type) break;
        if (++k == 5) return Statue::WIN;
    }

    // 左对角线
    k = 1;
    for (int i = 1; i < 5; ++i) {
        int preX = x - i;
        int preY = y - i;
        if (preX < 0 || preY < 0) break;
        if (chessData[preX][preY] != type) break;
        if (++k == 5) return Statue::WIN;
    }
    for (int i = 1; i < 5; ++i) {
        int nextX = x + i;
        int nextY = y + i;
        if (nextX >= rows || nextY >= cols) break;
        if (chessData[nextX][nextY] != type) break;
        if (++k == 5) return Statue::WIN;
    }

    // 右对角线
    k = 1;
    for (int i = 1; i < 5; ++i) {
        int nextX = x + i;
        int preY = y - i;
        if (nextX >= rows || preY < 0) break;
        if (chessData[nextX][preY] != type) break;
        if (++k == 5) return Statue::WIN;
    }
    for (int i = 1; i < 5; ++i) {
        int preX = x - i;
        int nextY = y + i;
        if (preX < 0 || nextY >= cols) break;
        if (chessData[preX][nextY] != type) break;
        if (++k == 5) return Statue::WIN;
    }

    // 检查是否平局
    for (const auto& row : chessData) {
        for (int v : row) {
            if (v == 0) return Statue::IN_PROGRESS;
        }
    }
    return Statue::DRAW;
}

// 对外接口：获取 AI 落子点
Point ZhiZhangAIService::getPoint(const std::vector<std::vector<int>>& chessData, const Point& point) {
    initChessData(chessData);
    statistics = Statistics();
    ai = 3 - point.type;
    bestPoint = Point();
    attack = (ai == 1) ? 1.8f : 0.5f;
    int depth = aiConfig.depth;

    // AI 先手，首子天元
    if (rounds == 1 && ai == 1) {
        int centerX = cols / 2;
        int centerY = rows / 2;
        return Point(centerX, centerY, ai);
    }

    // 深度小于2，使用普通单步评估
    if (aiConfig.depth < 2) {
        return getBestPoint(point);
    }

    // 高难度前三个回合降低深度
    if (aiConfig.depth > 4 && rounds < 4) {
        depth = 4;
    }

    // 算杀模式
    int vcx = aiConfig.vcx;
    if (vcx > 0) {
        int vcxDepth = aiConfig.vcxDepth;
        auto vcxStart = std::chrono::steady_clock::now();
        bestPoint = deepeningVcx(true, vcxDepth, vcx == 2);
        auto vcxEnd = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(vcxEnd - vcxStart).count();
        statistics.vcxTime = elapsed;
    }

    if (bestPoint.x == 0 && bestPoint.y == 0 && bestPoint.type == 0) {
        statistics.nodes = 0;
        statistics.cacheHits = 0;
        auto minimaxStart = std::chrono::steady_clock::now();
        bestPoint = deepeningMinimax(2, depth);
        auto minimaxEnd = std::chrono::steady_clock::now();
        statistics.minimaxTime = std::chrono::duration<double>(minimaxEnd - minimaxStart).count();
    }

    statistics.point = bestPoint;
    statistics.score = bestPoint.score;
    statistics.caches = static_cast<int>(situationCacheMap.size());

    if (aiConfig.debug) {
        std::cout << "============AI 统计[第" << rounds << "回合]==========" << std::endl;
        std::cout << "搜索深度：" << statistics.depth << std::endl;
        std::cout << "搜索节点数：" << statistics.nodes << std::endl;
        std::cout << "发生剪枝数：" << statistics.cuts << std::endl;
        std::cout << "缓存总数：" << statistics.caches << std::endl;
        std::cout << "缓存命中数：" << statistics.cacheHits << std::endl;
        if (vcx > 0) {
            std::cout << "算杀深度：" << statistics.vcxDepth << std::endl;
            std::cout << "算杀命中：" << (statistics.vcx == 1 ? "VCF" : (statistics.vcx == 2 ? "VCT" : "未命中")) << std::endl;
        }
        std::cout << "最佳落子点：" << statistics.point << std::endl;
        std::cout << "得分：" << statistics.score << std::endl;
        double totalTime = statistics.minimaxTime + statistics.vcxTime;
        std::cout << "耗时：" << totalTime << "s"
            << (vcx > 0 ? (", VCX(" + std::to_string(statistics.vcxTime) + "s)") : "")
            << ", MINIMAX(" << statistics.minimaxTime << "s)" << std::endl;
        std::cout << "==================================" << std::endl;
    }

    situationCacheMap.clear();
    return bestPoint;
}

// 单步最佳落子点（不搜索）
Point ZhiZhangAIService::getBestPoint(const Point& point) {
    Point best;
    int score = -INF;
    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            if (chessData[i][j] != 0) continue;
            Point p(i, j, ai);
            int val = static_cast<int>(evaluate(p) * attack) + evaluate(Point(i, j, 3 - ai));
            if (val > score) {
                score = val;
                best = p;
            }
        }
    }
    return best;
}

// 启发式获取落子点列表
std::vector<Point> ZhiZhangAIService::getHeuristicPoints(int type) {
    int max = aiConfig.maxNodes;
    std::vector<Point> highPriority, lowPriority, alternate, killPoints;
    int dangerLevel = 0;

    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            if (chessData[i][j] != 0) continue;

            Point p(i, j, type);
            int score = evaluate(p);
            if (score >= CHESS_MODELS[0].second.score) { // 连五分数
                return { p };
            }

            if (dangerLevel == 2) continue;

            if (score >= static_cast<int>(RiskScore::MEDIUM_RISK)) {
                killPoints.push_back(p);
            }

            Point foePoint(i, j, 3 - type);
            int foeScore = evaluate(foePoint);
            int level = 0;
            if (foeScore >= CHESS_MODELS[0].second.score) {
                level = 2;
            }
            else if (foeScore >= static_cast<int>(RiskScore::MEDIUM_RISK)) {
                level = 1;
            }

            if (level > 0) {
                if (dangerLevel < level) {
                    dangerLevel = level;
                    highPriority.clear();
                }
                highPriority.push_back(p);
                continue;
            }

            if (dangerLevel > 0) continue;

            if ((score >= static_cast<int>(RiskScore::LOW_RISK) && score < static_cast<int>(RiskScore::MEDIUM_RISK)) ||
                (foeScore >= static_cast<int>(RiskScore::LOW_RISK) && foeScore < static_cast<int>(RiskScore::MEDIUM_RISK))) {
                highPriority.push_back(p);
                continue;
            }

            if (highPriority.empty()) {
                if (score >= CHESS_MODELS[3].second.score || foeScore >= CHESS_MODELS[3].second.score) {
                    lowPriority.push_back(p);
                    continue;
                }
                if (lowPriority.empty() && score >= CHESS_MODELS[8].second.score) {
                    alternate.push_back(p);
                }
            }
        }
    }

    if (dangerLevel < 2 && !killPoints.empty()) {
        return killPoints;
    }

    std::vector<Point>* pointList = nullptr;
    if (!highPriority.empty()) {
        pointList = &highPriority;
    }
    else if (!lowPriority.empty()) {
        pointList = &lowPriority;
    }
    else if (!alternate.empty()) {
        std::shuffle(alternate.begin(), alternate.end(), RNG);
        pointList = &alternate;
    }
    else {
        return randomPoint(type, 1);
    }

    // 按分数降序排序
    std::sort(pointList->begin(), pointList->end(), [](const Point& a, const Point& b) {
        return a.score > b.score;
        });
    if (pointList->size() > static_cast<size_t>(max)) {
        pointList->resize(max);
    }
    return *pointList;
}

// 随机落子点
std::vector<Point> ZhiZhangAIService::randomPoint(int type, int num) {
    std::vector<Point> points;
    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            if (chessData[i][j] == 0) {
                points.emplace_back(i, j, type);
            }
        }
    }
    std::shuffle(points.begin(), points.end(), RNG);
    if (points.size() > static_cast<size_t>(num)) points.resize(num);
    return points;
}

// 极大极小搜索（无剪枝，但代码中实际未使用，保留接口）
int ZhiZhangAIService::minimax(int type, int depth) {
    bool isRoot = (type == 0);
    if (isRoot) type = ai;
    bool isAI = (type == ai);
    int score = isAI ? -INF : INF;

    if (depth == 0) {
        return evaluateAll();
    }

    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            if (chessData[i][j] != 0) continue;
            Point p(i, j, type);
            putChess(p);
            int curScore = minimax(3 - type, depth - 1);
            revokeChess(p);
            if (isAI) {
                if (curScore > score) {
                    score = curScore;
                    if (isRoot) bestPoint = p;
                }
            }
            else {
                if (curScore < score) score = curScore;
            }
        }
    }
    return score;
}

// Alpha-Beta 剪枝搜索
int ZhiZhangAIService::minimax(int type, int depth, int alpha, int beta) {
    bool isRoot = (type == 0);
    if (isRoot) type = ai;
    bool isAI = (type == ai);

    auto it = situationCacheMap.find(hashcode);
    if (it != situationCacheMap.end() && it->second.depth >= depth) {
        if (aiConfig.debug) statistics.incrCacheHits();
        return it->second.score;
    }

    if (depth == 0) {
        return evaluateAll();
    }

    std::vector<Point> pointList = getHeuristicPoints(type);
    if (isRoot && pointList.size() == 1) {
        if (aiConfig.debug) statistics.incrNodes();
        bestPoint = pointList[0];
        return bestPoint.score;
    }

    std::vector<Point> bestPointList;
    for (Point& point : pointList) {
        if (aiConfig.debug) statistics.incrNodes();
        if (point.score >= CHESS_MODELS[0].second.score) {
            point.score = isAI ? INF - 1 : -INF + 1;
        }
        else {
            putChess(point);
            point.score = minimax(3 - type, depth - 1, alpha, beta);
            revokeChess(point);
        }

        if (isAI) {
            if (point.score >= alpha) {
                if (isRoot) {
                    if (point.score > alpha && rounds <= 1) bestPointList.clear();
                    bestPointList.push_back(point);
                }
                alpha = point.score;
            }
        }
        else {
            if (point.score < beta) {
                beta = point.score;
            }
        }

        if (alpha >= beta) {
            if (aiConfig.debug) statistics.incrCuts();
            break;
        }
    }

    if (isRoot) {
        if (bestPointList.size() == 1) {
            bestPoint = bestPointList[0];
        }
        else if (rounds > 1) {
            bestPoint = getRandomBestPoint(bestPointList);
        }
        else {
            bestPoint = getBestPoint(bestPointList);
        }
    }

    int resultScore = isAI ? alpha : beta;
    situationCacheMap[hashcode] = SituationCache(resultScore, depth);
    return resultScore;
}

// 迭代加深 minimax
Point ZhiZhangAIService::deepeningMinimax(int depth, int maxDepth) {
    situationCacheMap.clear();
    Point best;
    for (; depth <= maxDepth; depth += 2) {
        int score = minimax(0, depth, -INF, INF);
        best = bestPoint;
        statistics.depth = depth;
        statistics.score = score;
        if (std::abs(score) >= INF - 1) break;
    }
    return best;
}

// 迭代加深算杀
Point ZhiZhangAIService::deepeningVcx(bool isAi, int maxDepth, bool isVcf) {
    ai = isAi ? ai : 3 - ai;
    Point point = deepening(1, maxDepth, isVcf);
    if (!isAi) {
        ai = 3 - ai;
        if (point.x != 0 || point.y != 0 || point.type != 0) point.type = ai;
    }
    return point;
}

Point ZhiZhangAIService::deepening(int depth, int maxDepth, bool isVcf) {
    situationCacheMap.clear();
    Point point;
    for (; depth <= maxDepth; depth += 2) {
        statistics.vcxDepth = depth;
        if (aiConfig.debug) pathStack = std::stack<Point>();
        point = vcx(0, depth, isVcf);
        if (point.x != 0 || point.y != 0 || point.type != 0) {
            if (aiConfig.debug && !bestPathStack.empty()) {
                std::cout << (isVcf ? "VCF" : "VCT") << "路径：";
                // 注意：stack 无法直接遍历，需要复制，这里简化
                // 原 Java 代码打印了 bestPathStack，为了简洁略过
            }
            statistics.vcx = isVcf ? 1 : 2;
            break;
        }
    }
    return point;
}

// 算杀核心
Point ZhiZhangAIService::vcx(int type, int depth, bool isVcf) {
    auto it = situationCacheMap.find(hashcode);
    if (it != situationCacheMap.end() && it->second.depth >= depth) {
        if (aiConfig.debug) statistics.incrCacheHits();
        return it->second.point;
    }

    if (depth == 0) return Point();

    bool isRoot = (type == 0);
    if (isRoot) type = ai;
    bool isAI = (type == ai);

    Point best;
    std::vector<Point> pointList = getVcxPoints(type, isVcf);
    for (Point point : pointList) {
        if (aiConfig.debug) {
            statistics.incrNodes();
            pathStack.push(point);
        }

        if (point.score >= static_cast<int>(RiskScore::HIGH_RISK)) {
            if (aiConfig.debug) {
                if (isAI) bestPathStack = pathStack;
                pathStack.pop();
            }
            return isAI ? point : Point();
        }

        putChess(point);
        Point res = vcx(3 - type, depth - 1, isVcf);
        revokeChess(point);

        if (aiConfig.debug) pathStack.pop();

        if (res.x == 0 && res.y == 0 && res.type == 0) {
            if (isAI) continue;
            else return Point();
        }

        best = point;
        if (isAI) break;
    }

    situationCacheMap[hashcode] = SituationCache(best, depth);
    return best;
}

// 获取算杀候选点
std::vector<Point> ZhiZhangAIService::getVcxPoints(int type, bool isVcf) {
    bool isAI = (type == ai);
    std::vector<Point> attackPoints, defensePoints, vcxPoints;
    bool isDanger = false;

    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            if (chessData[i][j] != 0) continue;

            Point p(i, j, type);
            int score = evaluate(p);
            if (score >= CHESS_MODELS[0].second.score) return { p };

            if (isDanger) continue;

            Point foePoint(i, j, 3 - type);
            int foeScore = evaluate(foePoint);
            if (foeScore >= CHESS_MODELS[0].second.score) {
                isDanger = true;
                defensePoints.clear();
                defensePoints.push_back(p);
                continue;
            }

            if (score >= static_cast<int>(RiskScore::MEDIUM_RISK)) {
                attackPoints.push_back(p);
                continue;
            }

            if (isAI) {
                if (checkSituation(p, { ChessModel::CHONGSI })) {
                    vcxPoints.push_back(p);
                }
                else if (!isVcf && checkSituation(p, { ChessModel::HUOSAN })) {
                    vcxPoints.push_back(p);
                }
            }
            else {
                if (!isVcf && (checkSituation(p, { ChessModel::CHONGSI }) || foeScore >= CHESS_MODELS[1].second.score)) {
                    defensePoints.push_back(p);
                }
            }
        }
    }

    std::vector<Point> result;
    if (!isDanger) {
        if (!attackPoints.empty()) {
            std::sort(attackPoints.begin(), attackPoints.end(), [](const Point& a, const Point& b) {
                return a.score > b.score;
                });
            if (isAI) return attackPoints;
            result.insert(result.end(), attackPoints.begin(), attackPoints.end());
        }
        if (!vcxPoints.empty()) {
            result.insert(result.end(), vcxPoints.begin(), vcxPoints.end());
        }
    }
    if (!defensePoints.empty()) {
        if (isAI) result.insert(result.end(), defensePoints.begin(), defensePoints.end());
        else result.insert(result.begin(), defensePoints.begin(), defensePoints.end());
    }
    return result;
}

// 检查某个落子是否形成指定棋型
bool ZhiZhangAIService::checkSituation(const Point& point, const std::vector<ChessModel>& models) {
    for (int dir = 1; dir <= 4; ++dir) {
        std::string sit = getSituation(point, dir);
        for (auto model : models) {
            if (checkSituation(sit, model)) return true;
        }
    }
    return false;
}

bool ZhiZhangAIService::checkSituation(const std::string& situation, ChessModel model) {
    for (const auto& entry : CHESS_MODELS) {
        if (entry.first == model) {
            for (const auto& val : entry.second.values) {
                if (situation.find(val) != std::string::npos) return true;
            }
            break;
        }
    }
    return false;
}

// 获取局势对应的棋型
ZhiZhangAIService::ChessModel ZhiZhangAIService::getChessModel(const std::string& situation) {
    for (const auto& entry : CHESS_MODELS) {
        for (const auto& val : entry.second.values) {
            if (situation.find(val) != std::string::npos) {
                return entry.first;
            }
        }
    }
    // 返回一个默认值，实际不会用到
    return ChessModel::MIANYI;
}

int ZhiZhangAIService::getScore(const std::string& situation) {
    // 按棋型优先级（从高到低）检查是否包含该模式
    for (const auto& entry : CHESS_MODELS) {
        for (const auto& pattern : entry.second.values) {
            if (situation.find(pattern) != std::string::npos) {
                return entry.second.score;
            }
        }
    }
    return 0;
}

// 获取某个方向的局势字符串
std::string ZhiZhangAIService::getSituation(const Point& point, int direction) {
    int dir = direction * 2 - 1;
    std::string sb;
    // 负方向四个
    for (int offset = 4; offset >= 1; --offset) appendChess(sb, point, dir, offset);
    sb.push_back('1');  // 当前棋子视为黑棋
    for (int offset = 1; offset <= 4; ++offset) appendChess(sb, point, dir + 1, offset);
    return sb;
}

// 拼接棋子
void ZhiZhangAIService::appendChess(std::string& sb, const Point& point, int direction, int offset) {
    int chess = relativePoint(point, direction, offset);
    if (chess != -1) {
        if (point.type == 2 && chess > 0) {
            chess = 3 - chess;
        }
        sb.push_back('0' + chess);
    }
}

// 获取相对位置的棋子
int ZhiZhangAIService::relativePoint(const Point& point, int direction, int offset) const {
    int x = point.x, y = point.y;
    switch (direction) {
    case 1: x -= offset; break;
    case 2: x += offset; break;
    case 3: y -= offset; break;
    case 4: y += offset; break;
    case 5: x += offset; y -= offset; break;
    case 6: x -= offset; y += offset; break;
    case 7: x -= offset; y -= offset; break;
    case 8: x += offset; y += offset; break;
    }
    if (x < 0 || y < 0 || x >= cols || y >= rows) return -1;
    return chessData[x][y];
}

// 评估单个落子的分数
int ZhiZhangAIService::evaluate(const Point& point) {
    int score = 0;
    int huosanTotal = 0, chongsiTotal = 0, tfTotal = 0;

    for (int dir = 1; dir <= 4; ++dir) {
        std::string situation = getSituation(point, dir);
        ChessModel model = getChessModel(situation);
        switch (model) {
        case ChessModel::HUOSAN:
            huosanTotal++;
            if (checkSituation(situation, ChessModel::CHONGSI)) tfTotal++;
            break;
        case ChessModel::CHONGSI:
            chongsiTotal++;
            break;
        default:
            break;
        }
        score += getScore(situation);
    }

    if (chongsiTotal > 1 || tfTotal > 1) {
        score += static_cast<int>(RiskScore::HIGH_RISK);
    }
    else if ((chongsiTotal > 0 && huosanTotal > 0) || (tfTotal > 0 && huosanTotal > 1)) {
        score += static_cast<int>(RiskScore::MEDIUM_RISK);
    }
    else if (huosanTotal > 1) {
        score += static_cast<int>(RiskScore::LOW_RISK);
    }

    const_cast<Point&>(point).score = score;
    return score;
}

// 评估整个棋盘（以AI角度）
int ZhiZhangAIService::evaluateAll() {
    int aiScore = 0, foeScore = 0;
    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            int type = chessData[i][j];
            if (type == 0) continue;
            int val = evaluate(Point(i, j, type));
            if (type == ai) aiScore += val;
            else foeScore += val;
        }
    }
    return static_cast<int>(aiScore * attack) - foeScore;
}

// 从候选列表中选取最佳点（简单分数评估）
Point ZhiZhangAIService::getBestPoint(const std::vector<Point>& pointList) {
    Point best;
    int bestScore = -INF;
    for (const Point& p : pointList) {
        int score = static_cast<int>(evaluate(p) * attack) + evaluate(Point(p.x, p.y, 3 - p.type));
        if (score > bestScore) {
            bestScore = score;
            best = p;
        }
    }
    return best;
}

// 随机选取最佳或次佳
Point ZhiZhangAIService::getRandomBestPoint(const std::vector<Point>& pointList) {
    Point best, second;
    int bestScore = -INF, secondScore = -INF;
    for (const Point& p : pointList) {
        int score = static_cast<int>(evaluate(p) * attack) + evaluate(Point(p.x, p.y, 3 - p.type));
        if (score > bestScore) {
            secondScore = bestScore;
            second = best;
            bestScore = score;
            best = p;
        }
        else if (score > secondScore && score < bestScore) {
            secondScore = score;
            second = p;
        }
    }
    if (second.x == 0 && second.y == 0) return best;
    // 随机选择
    static std::uniform_real_distribution<double> dist(0, 1);
    return (dist(RNG) < 0.5) ? best : second;
}

// 检查高优先级点（用于启发式，但原代码中未直接使用，保留）
bool ZhiZhangAIService::checkHighPriorityPoint(const Point& point) {
    int huosan = 0, chongsi = 0, huoer = 0;
    for (int i = 1; i <= 4; ++i) {
        std::string sit = getSituation(point, i);
        ChessModel model = getChessModel(sit);
        switch (model) {
        case ChessModel::HUOSI: return true;
        case ChessModel::HUOSAN: huosan++; break;
        case ChessModel::CHONGSI: chongsi++; break;
        case ChessModel::HUOER: huoer++; break;
        default: break;
        }
    }
    if (chongsi > 1 || (chongsi > 0 && huosan > 0)) return true;
    if (huosan > 1) return true;
    // if (huosan>0 && huoer>0) return true;  // 原代码注释掉
    return false;
}