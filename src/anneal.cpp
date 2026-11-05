// Simulated annealing over partitions for the "States" puzzle.
#include <vector>
#include <random>
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <string>
#include <functional>
using namespace std;

int N = 11, V = 121;
vector<int> clue;            // -1 or value
vector<vector<int>> adjl;
double LAMBDA = 6.0;

struct Op { int t, p1, p2; };
struct Eval {
    double E; int clueErr; int asym; vector<int> comp; vector<int> size; vector<int> caps; vector<int> d; vector<vector<Op>> ops; vector<vector<int>> comps;
};
vector<pair<int,int>> MINSIZE; // cell, min size
static vector<Op>* curOps = nullptr;

static inline bool inside(int r, int c) { return r >= 0 && r < N && c >= 0 && c < N; }

// apply op: type 0 R180 (P doubled), 1 R90 (P doubled), 2 H (A), 3 V (B), 4 D (k), 5 A (k)
static inline int img(int t, int p1, int p2, int r, int c) {
    int r1, c1;
    switch (t) {
        case 0: r1 = p1 - r; c1 = p2 - c; break;
        case 1: r1 = (p1 - p2) / 2 + c; c1 = (p1 + p2) / 2 - r; break;
        case 2: r1 = p1 - r; c1 = c; break;
        case 3: r1 = r; c1 = p1 - c; break;
        case 4: r1 = c - p1; c1 = r + p1; break;
        default: r1 = p1 - c; c1 = p1 - r; break;
    }
    if (!inside(r1, c1)) return -1;
    return r1 * N + c1;
}

static vector<int> mark;  // scratch: comp id membership
static int markStamp = 0;
static vector<int> stamp;

// returns asymmetry count, sets cap (-1 if none)
int analyse(const vector<int>& cells, int &cap) {
    int n = cells.size();
    long sr = 0, sc = 0;
    markStamp++;
    for (int v : cells) { sr += v / N; sc += v % N; stamp[v] = markStamp; }
    auto mism = [&](int t, int p1, int p2) {
        int m = 0;
        for (int v : cells) { int w = img(t, p1, p2, v / N, v % N); if (w < 0 || stamp[w] != markStamp) m++; }
        return m;
    };
    int best = n;
    // approximate centers
    double R2 = 2.0 * sr / n, C2 = 2.0 * sc / n;
    int rlo = (int)floor(R2), clo = (int)floor(C2);
    for (int a = rlo; a <= rlo + 1; a++) for (int b = clo; b <= clo + 1; b++) {
        best = min(best, mism(0, a, b));
        if (((a - b) % 2 + 2) % 2 == 0) best = min(best, mism(1, a, b));
    }
    for (int a = rlo; a <= rlo + 1; a++) best = min(best, mism(2, a, 0));
    for (int b = clo; b <= clo + 1; b++) best = min(best, mism(3, b, 0));
    double kd = (double)(sc - sr) / n, ka = (double)(sr + sc) / n;
    for (int k = (int)floor(kd); k <= (int)floor(kd) + 1; k++) best = min(best, mism(4, k, 0));
    for (int k = (int)floor(ka); k <= (int)floor(ka) + 1; k++) best = min(best, mism(5, k, 0));
    cap = -1;
    if (best > 0) return best;
    // exact symmetric: determine capitol
    vector<int> fixedCells; bool ok = true; int nsym = 0; int capc = -1;
    auto consider = [&](int t, int p1, int p2) {
        if (mism(t, p1, p2) != 0) return;
        nsym++; if (curOps) curOps->push_back({t, p1, p2});
        int f = 0, fc = -1;
        for (int v : cells) if (img(t, p1, p2, v / N, v % N) == v) { f++; fc = v; }
        if (f != 1) ok = false; else { if (capc == -1) capc = fc; else if (capc != fc) ok = false; }
    };
    bool rI = (2 * sr) % n == 0, cI = (2 * sc) % n == 0;
    if (rI && cI) {
        int P1 = 2 * sr / n, P2 = 2 * sc / n;
        consider(0, P1, P2);
        if ((P1 - P2) % 2 == 0) consider(1, P1, P2);
    }
    if (rI) consider(2, 2 * sr / n, 0);
    if (cI) consider(3, 2 * sc / n, 0);
    if ((sc - sr) % n == 0) consider(4, (sc - sr) / n, 0);
    if ((sr + sc) % n == 0) consider(5, (sr + sc) / n, 0);
    if (n == 1) { cap = cells[0]; return 0; }
    if (nsym > 0 && ok && capc >= 0) cap = capc;
    return 0;
}

Eval evaluate(const vector<int>& lab) {
    Eval e; e.comp.assign(V, -1);
    vector<vector<int>> comps;
    for (int v = 0; v < V; v++) if (e.comp[v] < 0) {
        int id = comps.size(); comps.push_back({});
        vector<int> st = {v}; e.comp[v] = id;
        while (!st.empty()) { int x = st.back(); st.pop_back(); comps[id].push_back(x);
            for (int y : adjl[x]) if (e.comp[y] < 0 && lab[y] == lab[x]) { e.comp[y] = id; st.push_back(y); } }
    }
    e.size.assign(V, 0); e.asym = 0; e.caps.clear();
    e.ops.assign(comps.size(), {});
    for (size_t ci = 0; ci < comps.size(); ci++) { auto &cs = comps[ci]; int cap; curOps = &e.ops[ci]; int a = analyse(cs, cap); curOps = nullptr; e.asym += a; if (cap >= 0 && a == 0) e.caps.push_back(cap); for (int v : cs) e.size[v] = cs.size(); }
    // dijkstra
    const int INF = 1 << 28;
    e.d.assign(V, INF);
    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>> pq;
    for (int c : e.caps) { e.d[c] = 0; pq.push({0, c}); }
    while (!pq.empty()) { auto [dd, u] = pq.top(); pq.pop(); if (dd > e.d[u]) continue;
        for (int w : adjl[u]) { int nd = dd + min(e.size[u], e.size[w]); if (nd < e.d[w]) { e.d[w] = nd; pq.push({nd, w}); } } }
    e.clueErr = 0;
    for (int v = 0; v < V; v++) if (clue[v] >= 0) e.clueErr += min(abs(min(e.d[v], 400) - clue[v]), 400);
    e.E = e.clueErr + LAMBDA * e.asym;
    for (auto &pr : MINSIZE) if (e.size[pr.first] < pr.second) e.E += 2.0 * (pr.second - e.size[pr.first]);
    e.comps = comps;
    return e;
}

int main(int argc, char** argv) {
    // args: cluefile seed moves T0 T1
    const char* cf = argv[1]; unsigned seed = atoi(argv[2]); long moves = atol(argv[3]);
    double T0 = atof(argv[4]), T1 = atof(argv[5]);
    if (argc > 6) LAMBDA = atof(argv[6]);
    for (int a = 7; a + 2 < argc; a += 3) MINSIZE.push_back({atoi(argv[a]) * 11 + atoi(argv[a+1]), atoi(argv[a+2])});
    FILE* f = fopen(cf, "r"); fscanf(f, "%d", &N); V = N * N; clue.assign(V, -1);
    int r, c, val; while (fscanf(f, "%d %d %d", &r, &c, &val) == 3) clue[r * N + c] = val; fclose(f);
    adjl.assign(V, {}); stamp.assign(V, 0);
    for (int v = 0; v < V; v++) { int r0 = v / N, c0 = v % N; int dr[4] = {1,-1,0,0}, dc[4] = {0,0,1,-1};
        for (int k = 0; k < 4; k++) if (inside(r0+dr[k], c0+dc[k])) adjl[v].push_back((r0+dr[k])*N + c0+dc[k]); }
    mt19937_64 rng(seed);
    vector<int> lab(V); for (int v = 0; v < V; v++) lab[v] = v;
    int nextLab = V;
    Eval cur = evaluate(lab), best = cur; vector<int> bestLab = lab;
    uniform_real_distribution<double> U(0, 1);
    for (long it = 0; it < moves; it++) {
        double T = T0 * pow(T1 / T0, (double)it / moves);
        int v = rng() % V;
        vector<int> saved = lab;
        double rr = U(rng);
        int u = adjl[v][rng() % adjl[v].size()];
        if (rr < 0.03) { lab[v] = nextLab++; }
        else if (rr < 0.08) {
            // merge whole state of v into u's state
            if (lab[u] == lab[v]) continue;
            int L = lab[v]; for (int x = 0; x < V; x++) if (lab[x] == L) lab[x] = lab[u];
        } else if (rr < 0.40) {
            // symmetric add: v (and its image under an exact op of u's state) join u's state
            if (lab[u] == lab[v]) continue;
            int ci = cur.comp[u]; auto &ops = cur.ops[ci];
            lab[v] = lab[u];
            if (!ops.empty()) { Op o = ops[rng() % ops.size()]; int w = img(o.t, o.p1, o.p2, v / N, v % N); if (w >= 0) lab[w] = lab[u]; }
        } else if (rr < 0.55) {
            // symmetric remove: v and its image leave v's state, join a random neighbour each
            int ci = cur.comp[v]; auto &ops = cur.ops[ci];
            if (lab[u] == lab[v]) { lab[v] = nextLab++; } else lab[v] = lab[u];
            if (!ops.empty()) { Op o = ops[rng() % ops.size()]; int w = img(o.t, o.p1, o.p2, v / N, v % N);
                if (w >= 0 && w != v) { int u2 = adjl[w][rng() % adjl[w].size()]; if (saved[u2] != saved[w]) lab[w] = saved[u2]; else lab[w] = nextLab++; } }
        } else {
            if (lab[u] == lab[v]) continue;
            lab[v] = lab[u];
        }
        Eval nx = evaluate(lab);
        double dE = nx.E - cur.E;
        if (dE <= 0 || U(rng) < exp(-dE / T)) {
            cur = nx;
            if (cur.E < best.E) { best = cur; bestLab = lab; }
        } else {
            lab = saved;
        }
        if (it % 2000000 == 0) { fprintf(stderr, "it %ld T %.3f cur %.1f (err %d asym %d) best %.1f\n", it, T, cur.E, cur.clueErr, cur.asym, best.E); }
        if (best.E == 0) break;
    }
    printf("BEST %.1f err %d asym %d\n", best.E, best.clueErr, best.asym);
    for (int r0 = 0; r0 < N; r0++) { for (int c0 = 0; c0 < N; c0++) printf("%3d", best.size[r0*N+c0]); printf("\n"); }
    printf("comp:");
    for (int v = 0; v < V; v++) printf(" %d", best.comp[v]); printf("\n");
    printf("lab:"); for (int v = 0; v < V; v++) printf(" %d", bestLab[v]); printf("\n");
    printf("caps:"); for (int x : best.caps) printf(" %d", x); printf("\n");
    return 0;
}
