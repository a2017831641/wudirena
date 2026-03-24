#pragma once
#include <bits/stdc++.h>
using namespace std;

constexpr double kInf = 1e18;
constexpr double kPi  = 3.14159265358979323846;

inline double deg2rad(double d) { return d * kPi / 180.0; }
inline double rad2deg(double r) { return r * 180.0 / kPi; }

inline double normalizeAngle(double a) {
    while (a <= -kPi) a += 2.0 * kPi;
    while (a >   kPi) a -= 2.0 * kPi;
    return a;
}

struct Pose {
    double x = 0.0;
    double y = 0.0;
    double yaw = 0.0; // rad
};

struct MotionPrimitive {
    double steer = 0.0; // rad
    int dir = 1;        // +1 only for forward version
    double step = 0.8;  // meter-like grid unit
};

struct Node3D {
    Pose pose;          // continuous (x, y, yaw)
    double g = kInf;
    double h = 0.0;
    int parent = -1;
    int dir = 1;
    double steer = 0.0;
};

inline vector<MotionPrimitive> buildForwardMotions(double max_steer, double step) {
    vector<MotionPrimitive> motions;
    motions.reserve(5);

    for (int i = 0; i < 5; ++i) {
        double t = -1.0 + 0.5 * i; // -1, -0.5, 0, 0.5, 1
        MotionPrimitive m;
        m.steer = t * max_steer;
        m.dir = 1;
        m.step = step;
        motions.push_back(m);
    }

    return motions;
}

inline Pose propagateForward(const Pose& cur, const MotionPrimitive& motion, double wheel_base) {
    Pose nxt = cur;
    double ds = motion.step;
    double beta = tan(motion.steer);

    nxt.x = cur.x + ds * cos(cur.yaw);
    nxt.y = cur.y + ds * sin(cur.yaw);
    nxt.yaw = normalizeAngle(cur.yaw + ds * beta / wheel_base);

    return nxt;
}

struct GridMap {
    int h = 0, w = 0;
    vector<string> raw; // 原始地图
    vector<string> occ; // 实际用于搜索/碰撞检测的地图（可做膨胀）

    bool inBoundsRC(int r, int c) const {
        return 0 <= r && r < h && 0 <= c && c < w;
    }

    bool isBlockedRC(int r, int c) const {
        if (!inBoundsRC(r, c)) return true;
        return occ[r][c] == '#';
    }

    pair<int, int> xyToCell(double x, double y) const {
        int c = static_cast<int>(floor(x));
        int r = static_cast<int>(floor(y));
        return {r, c};
    }

    pair<double, double> cellCenter(int r, int c) const {
        return {c + 0.5, r + 0.5};
    }

    bool isBlockedXY(double x, double y) const {
        pair<int, int> rc = xyToCell(x, y);
        return isBlockedRC(rc.first, rc.second);
    }
};

struct Problem {
    GridMap map;
    Pose start;
    Pose goal;
};

inline Problem readProblem(istream& in) {
    Problem p;
    in >> p.map.h >> p.map.w;

    if (p.map.h <= 0 || p.map.w <= 0) {
        throw runtime_error("invalid map size");
    }

    p.map.raw.resize(p.map.h);
    for (int r = 0; r < p.map.h; ++r) {
        in >> p.map.raw[r];
        if ((int)p.map.raw[r].size() != p.map.w) {
            throw runtime_error("grid width mismatch at row " + to_string(r));
        }
    }

    p.map.occ = p.map.raw;

    double syaw_deg, gyaw_deg;
    in >> p.start.x >> p.start.y >> syaw_deg;
    in >> p.goal.x  >> p.goal.y  >> gyaw_deg;

    p.start.yaw = deg2rad(syaw_deg);
    p.goal.yaw  = deg2rad(gyaw_deg);

    return p;
}

inline void inflateObstacles(GridMap& m, int radius_cells) {
    m.occ = m.raw;
    if (radius_cells <= 0) return;

    vector<string> out = m.raw;
    for (int r = 0; r < m.h; ++r) {
        for (int c = 0; c < m.w; ++c) {
            if (m.raw[r][c] != '#') continue;

            for (int dr = -radius_cells; dr <= radius_cells; ++dr) {
                for (int dc = -radius_cells; dc <= radius_cells; ++dc) {
                    if (dr * dr + dc * dc > radius_cells * radius_cells) continue;
                    int nr = r + dr, nc = c + dc;
                    if (m.inBoundsRC(nr, nc)) {
                        out[nr][nc] = '#';
                    }
                }
            }
        }
    }
    m.occ.swap(out);
}

inline void printAsciiMap(
    const GridMap& m,
    const vector<pair<int,int>>& path = {},
    const Pose* start = nullptr,
    const Pose* goal = nullptr
) {
    vector<string> canvas = m.occ;

    for (size_t i = 0; i < path.size(); ++i) {
        int r = path[i].first;
        int c = path[i].second;
        if (m.inBoundsRC(r, c) && canvas[r][c] == '.') {
            canvas[r][c] = '*';
        }
    }

    if (start) {
        pair<int, int> src = m.xyToCell(start->x, start->y);
        if (m.inBoundsRC(src.first, src.second)) canvas[src.first][src.second] = 'S';
    }

    if (goal) {
        pair<int, int> grc = m.xyToCell(goal->x, goal->y);
        if (m.inBoundsRC(grc.first, grc.second)) canvas[grc.first][grc.second] = 'G';
    }

    for (size_t i = 0; i < canvas.size(); ++i) {
        cout << canvas[i] << "\n";
    }
}
