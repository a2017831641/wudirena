#include "../helper/helper.hpp"

struct PQ2D {
    double d;
    int r;
    int c;

    bool operator<(const PQ2D& other) const {
        return d > other.d;
    }
};

vector<vector<double>> build2DDistanceMap(const GridMap& m, pair<int, int> goal) {
    const vector<pair<int, int>> dirs = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    vector<vector<double>> dist(m.h, vector<double>(m.w, kInf));
    priority_queue<PQ2D> pq;

    if (!m.isBlockedRC(goal.first, goal.second)) {
        dist[goal.first][goal.second] = 0.0;
        pq.push(PQ2D{0.0, goal.first, goal.second});
    }

    while (!pq.empty()) {
        PQ2D cur = pq.top();
        pq.pop();

        if (cur.d > dist[cur.r][cur.c] + 1e-12) continue;

        for (size_t i = 0; i < dirs.size(); ++i) {
            int dr = dirs[i].first;
            int dc = dirs[i].second;
            int nr = cur.r + dr;
            int nc = cur.c + dc;

            if (!m.inBoundsRC(nr, nc)) continue;
            if (m.isBlockedRC(nr, nc)) continue;

            if (abs(dr) == 1 && abs(dc) == 1) {
                if (m.isBlockedRC(cur.r, nc) || m.isBlockedRC(nr, cur.c)) continue;
            }

            double w = (abs(dr) + abs(dc) == 2) ? sqrt(2.0) : 1.0;
            double nd = cur.d + w;
            if (nd + 1e-12 < dist[nr][nc]) {
                dist[nr][nc] = nd;
                pq.push(PQ2D{nd, nr, nc});
            }
        }
    }

    return dist;
}

int yawToBin(double yaw, int yaw_bins) {
    double a = normalizeAngle(yaw);
    double t = (a + kPi) / (2.0 * kPi);
    int bin = static_cast<int>(floor(t * yaw_bins));
    if (bin < 0) bin = 0;
    if (bin >= yaw_bins) bin = yaw_bins - 1;
    return bin;
}

struct OpenItem {
    double f;
    int node_id;

    bool operator<(const OpenItem& other) const {
        return f > other.f;
    }
};

bool isPoseCollisionFree(const GridMap& m, const Pose& p) {
    if (p.x < 0.0 || p.y < 0.0 || p.x >= m.w || p.y >= m.h) return false;
    return !m.isBlockedXY(p.x, p.y);
}

bool isMotionCollisionFree(const GridMap& m, const Pose& from, const MotionPrimitive& motion, double wheel_base) {
    const int checks = 6;
    double sub_step = motion.step / checks;
    MotionPrimitive sub_motion;
    sub_motion.steer = motion.steer;
    sub_motion.dir = motion.dir;
    sub_motion.step = sub_step;

    Pose cur = from;
    for (int i = 0; i < checks; ++i) {
        cur = propagateForward(cur, sub_motion, wheel_base);
        if (!isPoseCollisionFree(m, cur)) return false;
    }
    return true;
}

vector<Pose> reconstructPosePath(const vector<Node3D>& nodes, int goal_id) {
    vector<Pose> path;
    int cur = goal_id;
    while (cur != -1) {
        path.push_back(nodes[cur].pose);
        cur = nodes[cur].parent;
    }
    reverse(path.begin(), path.end());
    return path;
}

vector<Pose> hybridAStarForward(
    const GridMap& m,
    const Pose& start,
    const Pose& goal,
    const vector<vector<double>>& h2d
) {
    const int yaw_bins = 72;
    const double wheel_base = 1.0;
    const double step = 0.8;
    const double max_steer = deg2rad(35.0);
    const double goal_pos_tol = 0.75;
    const double goal_yaw_tol = deg2rad(20.0);

    vector<MotionPrimitive> motions = buildForwardMotions(max_steer, step);

    vector<double> best_g(m.h * m.w * yaw_bins, kInf);
    auto stateIndex = [&](int r, int c, int yb) {
        return (r * m.w + c) * yaw_bins + yb;
    };

    vector<Node3D> nodes;
    nodes.reserve(200000);
    priority_queue<OpenItem> open;

    pair<int, int> s_rc = m.xyToCell(start.x, start.y);
    pair<int, int> g_rc = m.xyToCell(goal.x, goal.y);

    if (!m.inBoundsRC(s_rc.first, s_rc.second) || !m.inBoundsRC(g_rc.first, g_rc.second)) return {};

    int s_yb = yawToBin(start.yaw, yaw_bins);
    double sh = h2d[s_rc.first][s_rc.second];
    if (sh >= kInf / 2) return {};

    Node3D s;
    s.pose = start;
    s.g = 0.0;
    s.h = sh;
    s.parent = -1;
    s.dir = 1;
    s.steer = 0.0;

    nodes.push_back(s);
    best_g[stateIndex(s_rc.first, s_rc.second, s_yb)] = 0.0;
    open.push(OpenItem{s.g + s.h, 0});

    while (!open.empty()) {
        OpenItem top = open.top();
        open.pop();

        Node3D cur = nodes[top.node_id];
        pair<int, int> cur_rc = m.xyToCell(cur.pose.x, cur.pose.y);
        if (!m.inBoundsRC(cur_rc.first, cur_rc.second)) continue;

        int cur_bin = yawToBin(cur.pose.yaw, yaw_bins);
        int cur_idx = stateIndex(cur_rc.first, cur_rc.second, cur_bin);
        if (cur.g > best_g[cur_idx] + 1e-12) continue;

        double dx = cur.pose.x - goal.x;
        double dy = cur.pose.y - goal.y;
        double dyaw = normalizeAngle(cur.pose.yaw - goal.yaw);
        if (hypot(dx, dy) <= goal_pos_tol && fabs(dyaw) <= goal_yaw_tol) {
            return reconstructPosePath(nodes, top.node_id);
        }

        for (size_t i = 0; i < motions.size(); ++i) {
            const MotionPrimitive& motion = motions[i];
            Pose next_pose = propagateForward(cur.pose, motion, wheel_base);

            if (!isMotionCollisionFree(m, cur.pose, motion, wheel_base)) continue;

            pair<int, int> nr = m.xyToCell(next_pose.x, next_pose.y);
            if (!m.inBoundsRC(nr.first, nr.second)) continue;
            if (m.isBlockedRC(nr.first, nr.second)) continue;

            double h = h2d[nr.first][nr.second];
            if (h >= kInf / 2) continue;

            double steer_penalty = 0.03 * fabs(motion.steer);
            double ng = cur.g + motion.step + steer_penalty;
            int nyb = yawToBin(next_pose.yaw, yaw_bins);
            int nidx = stateIndex(nr.first, nr.second, nyb);

            if (ng + 1e-12 >= best_g[nidx]) continue;

            best_g[nidx] = ng;
            Node3D nxt;
            nxt.pose = next_pose;
            nxt.g = ng;
            nxt.h = h;
            nxt.parent = top.node_id;
            nxt.dir = 1;
            nxt.steer = motion.steer;

            nodes.push_back(nxt);
            int nid = static_cast<int>(nodes.size()) - 1;
            open.push(OpenItem{ng + h, nid});
        }
    }

    return {};
}

vector<pair<int, int>> posePathToCells(const GridMap& m, const vector<Pose>& path) {
    vector<pair<int, int>> cells;
    cells.reserve(path.size());

    for (size_t i = 0; i < path.size(); ++i) {
        pair<int, int> rc = m.xyToCell(path[i].x, path[i].y);
        if (!m.inBoundsRC(rc.first, rc.second)) continue;
        if (cells.empty() || cells.back() != rc) cells.push_back(rc);
    }

    return cells;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    try {
        Problem prob = readProblem(cin);

        int inflation_radius = 0;
        inflateObstacles(prob.map, inflation_radius);

        pair<int, int> start_rc = prob.map.xyToCell(prob.start.x, prob.start.y);
        pair<int, int> goal_rc = prob.map.xyToCell(prob.goal.x, prob.goal.y);

        cout << fixed << setprecision(2);

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
            printAsciiMap(prob.map, vector<pair<int, int> >(), &prob.start, &prob.goal);
            return 0;
        }
        if (prob.map.isBlockedRC(goal_rc.first, goal_rc.second)) {
            cout << "ERROR: GOAL_IN_OBSTACLE\n";
            printAsciiMap(prob.map, vector<pair<int, int> >(), &prob.start, &prob.goal);
            return 0;
        }

        vector<vector<double>> h2d = build2DDistanceMap(prob.map, goal_rc);
        vector<Pose> path = hybridAStarForward(prob.map, prob.start, prob.goal, h2d);

        if (path.empty()) {
            cout << "NO_PATH\n";
            printAsciiMap(prob.map, vector<pair<int, int> >(), &prob.start, &prob.goal);
            return 0;
        }

        for (size_t i = 0; i < path.size(); ++i) {
            cout << path[i].x << " " << path[i].y << "\n";
        }

        vector<pair<int, int>> cell_path = posePathToCells(prob.map, path);
        printAsciiMap(prob.map, cell_path, &prob.start, &prob.goal);
    } catch (const exception& e) {
        cout << "EXCEPTION: " << e.what() << "\n";
    }

    return 0;
}
