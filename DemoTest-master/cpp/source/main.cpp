#include "../helper/helper.hpp"

struct PQNode2D {
    double f;
    int r, c;

    bool operator<(const PQNode2D& other) const {
        return f > other.f; // priority_queue 默认大顶堆，这里反转成小顶堆效果
    }
};

double octileHeuristic(int r1, int c1, int r2, int c2) {
    int dr = abs(r1 - r2);
    int dc = abs(c1 - c2);
    int mn = min(dr, dc);
    int mx = max(dr, dc);
    return mn * sqrt(2.0) + (mx - mn);
}

vector<pair<int,int>> reconstructPath2D(
    pair<int,int> goal,
    const vector<vector<pair<int,int>>>& parent
) {
    vector<pair<int,int>> path;
    auto cur = goal;

    while (cur.first != -1) {
        path.push_back(cur);
        cur = parent[cur.first][cur.second];
    }

    reverse(path.begin(), path.end());
    return path;
}

vector<pair<int,int>> astar2D(
    const GridMap& m,
    pair<int,int> start,
    pair<int,int> goal
) {
    const vector<pair<int,int>> dirs = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1,-1}, {-1, 1}, {1,-1}, {1, 1}
    };

    vector<vector<double>> g(m.h, vector<double>(m.w, kInf));
    vector<vector<char>> closed(m.h, vector<char>(m.w, 0));
    vector<vector<pair<int,int>>> parent(
        m.h, vector<pair<int,int>>(m.w, {-1, -1})
    );

    priority_queue<PQNode2D> pq;
    g[start.first][start.second] = 0.0;
    pq.push({octileHeuristic(start.first, start.second, goal.first, goal.second),
             start.first, start.second});

    while (!pq.empty()) {
        auto cur = pq.top();
        pq.pop();

        int r = cur.r;
        int c = cur.c;

        if (closed[r][c]) continue;
        closed[r][c] = 1;

        if (r == goal.first && c == goal.second) {
            return reconstructPath2D(goal, parent);
        }

        for (auto [dr, dc] : dirs) {
            int nr = r + dr;
            int nc = c + dc;

            if (!m.inBoundsRC(nr, nc)) continue;
            if (m.isBlockedRC(nr, nc)) continue;

            // 防止“沿对角穿墙”
            if (abs(dr) == 1 && abs(dc) == 1) {
                if (m.isBlockedRC(r, nc) || m.isBlockedRC(nr, c)) {
                    continue;
                }
            }

            double step_cost = (abs(dr) + abs(dc) == 2) ? sqrt(2.0) : 1.0;
            double ng = g[r][c] + step_cost;

            if (ng + 1e-12 >= g[nr][nc]) continue;

            g[nr][nc] = ng;
            parent[nr][nc] = {r, c};
            double nf = ng + octileHeuristic(nr, nc, goal.first, goal.second);
            pq.push({nf, nr, nc});
        }
    }

    return {};
}

vector<vector<double>> build2DDistanceMap(
    const GridMap& m,
    pair<int,int> goal
) {
    struct State {
        double d;
        int r, c;

        bool operator<(const State& other) const {
            return d > other.d;
        }
    };

    const vector<pair<int,int>> dirs = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1,-1}, {-1, 1}, {1,-1}, {1, 1}
    };

    vector<vector<double>> dist(m.h, vector<double>(m.w, kInf));
    priority_queue<State> pq;

    if (!m.isBlockedRC(goal.first, goal.second)) {
        dist[goal.first][goal.second] = 0.0;
        pq.push({0.0, goal.first, goal.second});
    }

    while (!pq.empty()) {
        auto cur = pq.top();
        pq.pop();

        if (cur.d > dist[cur.r][cur.c] + 1e-12) continue;

        for (auto [dr, dc] : dirs) {
            int nr = cur.r + dr;
            int nc = cur.c + dc;

            if (!m.inBoundsRC(nr, nc)) continue;
            if (m.isBlockedRC(nr, nc)) continue;

            // 防止对角穿墙
            if (abs(dr) == 1 && abs(dc) == 1) {
                if (m.isBlockedRC(cur.r, nc) || m.isBlockedRC(nr, cur.c)) {
                    continue;
                }
            }

            double w = (abs(dr) + abs(dc) == 2) ? sqrt(2.0) : 1.0;
            double nd = cur.d + w;

            if (nd + 1e-12 < dist[nr][nc]) {
                dist[nr][nc] = nd;
                pq.push({nd, nr, nc});
            }
        }
    }

    return dist;
}

void printDistanceMap(
    const GridMap& m,
    const vector<vector<double>>& dist
) {
    cout << "\n=== 2D DISTANCE MAP (from goal) ===\n";
    for (int r = 0; r < m.h; ++r) {
        for (int c = 0; c < m.w; ++c) {
            if (m.isBlockedRC(r, c)) {
                cout << setw(8) << "#";
            } else if (dist[r][c] >= kInf / 2) {
                cout << setw(8) << "INF";
            } else {
                cout << setw(8) << fixed << setprecision(1) << dist[r][c];
            }
        }
        cout << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    try {
        Problem prob = readProblem(cin);

        // Step 0：先设为 0，确保你最容易调试成功
        // 后面如果你想模拟“机器人有半径”，再改成 1、2 ...
        int inflation_radius = 0;
        inflateObstacles(prob.map, inflation_radius);

        auto start_rc = prob.map.xyToCell(prob.start.x, prob.start.y);
        auto goal_rc  = prob.map.xyToCell(prob.goal.x, prob.goal.y);

        cout << fixed << setprecision(2);
        cout << "MAP_SIZE " << prob.map.h << " " << prob.map.w << "\n";
        cout << "START " << prob.start.x << " " << prob.start.y << " "
             << rad2deg(prob.start.yaw) << "deg"
             << " -> cell(" << start_rc.first << "," << start_rc.second << ")\n";
        cout << "GOAL  " << prob.goal.x << " " << prob.goal.y << " "
             << rad2deg(prob.goal.yaw) << "deg"
             << " -> cell(" << goal_rc.first << "," << goal_rc.second << ")\n";

        if (!prob.map.inBoundsRC(start_rc.first, start_rc.second)) {
            cout << "ERROR: START_OUT_OF_MAP\n";
            return 0;
        }
        if (!prob.map.inBoundsRC(goal_rc.first, goal_rc.second)) {
            cout << "ERROR: GOAL_OUT_OF_MAP\n";
            return 0;
        }

        if (prob.map.isBlockedRC(start_rc.first, start_rc.second)) {
            cout << "ERROR: START_IN_OBSTACLE\n";
            cout << "\n=== ASCII MAP ===\n";
            printAsciiMap(prob.map, {}, &prob.start, &prob.goal);
            return 0;
        }
        if (prob.map.isBlockedRC(goal_rc.first, goal_rc.second)) {
            cout << "ERROR: GOAL_IN_OBSTACLE\n";
            cout << "\n=== ASCII MAP ===\n";
            printAsciiMap(prob.map, {}, &prob.start, &prob.goal);
            return 0;
        }

        auto path2d = astar2D(prob.map, start_rc, goal_rc);
        auto dist2d = build2DDistanceMap(prob.map, goal_rc);

        if (path2d.empty()) {
            cout << "NO_PATH\n";
            cout << "\n=== ASCII MAP ===\n";
            printAsciiMap(prob.map, {}, &prob.start, &prob.goal);
            cout << "\n";
            printDistanceMap(prob.map, dist2d);
            return 0;
        }

        cout << "2D_PATH_CELL_COUNT " << path2d.size() << "\n";
        cout << "=== 2D PATH (cell centers) ===\n";
        for (auto [r, c] : path2d) {
        auto [x, y] = prob.map.cellCenter(r, c);
        cout << fixed << setprecision(2) << x << " " << y << "\n";
        }


        cout << "\n=== ASCII MAP ===\n";
        printAsciiMap(prob.map, path2d, &prob.start, &prob.goal);

        cout << "\n";
        printDistanceMap(prob.map, dist2d);
    }
    catch (const exception& e) {
        cout << "EXCEPTION: " << e.what() << "\n";
    }

    return 0;
}
