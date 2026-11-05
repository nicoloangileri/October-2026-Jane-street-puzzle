"""Check a division into states against the rules of "It's a Metric, Too".

Runs on the worked example (expected 8,045) and on the 11-by-11 grid (expected
408,951). For each grid it checks connectivity, symmetry, capitols, every clue
and the final sum of squares of the row sums.
"""
import heapq


def parse(rows):
    return {(r, c): ch for r, row in enumerate(rows) for c, ch in enumerate(row.split())}


def states_of(colours):
    """Group edge-connected cells of equal colour into states."""
    seen, states = set(), []
    for start in colours:
        if start in seen:
            continue
        stack, cells = [start], set()
        seen.add(start)
        while stack:
            r, c = stack.pop()
            cells.add((r, c))
            for n in ((r + 1, c), (r - 1, c), (r, c + 1), (r, c - 1)):
                if n in colours and n not in seen and colours[n] == colours[start]:
                    seen.add(n)
                    stack.append(n)
        states.append(frozenset(cells))
    return states


# The seven nontrivial symmetries of the square, about the origin.
OPS = [
    lambda x, y: (-y, x), lambda x, y: (-x, -y), lambda x, y: (y, -x),
    lambda x, y: (-x, y), lambda x, y: (x, -y), lambda x, y: (y, x), lambda x, y: (-y, -x),
]


def symmetries(cells):
    """Return, for each nontrivial symmetry, the squares it leaves fixed."""
    found = []
    sumr = sum(r for r, _ in cells)
    sumc = sum(c for _, c in cells)
    n = len(cells)
    # Work at scale 2n so that the centroid has integer coordinates.
    pts = {(2 * n * r - 2 * sumr, 2 * n * c - 2 * sumc): (r, c) for r, c in cells}
    for op in OPS:
        if {op(*p) for p in pts} == set(pts):
            found.append([cell for p, cell in pts.items() if op(*p) == p])
    return found


def capitol(cells):
    syms = symmetries(cells)
    assert syms, f"state is not symmetric: {sorted(cells)}"
    fixed = {tuple(f) for f in syms}
    if all(len(f) == 1 for f in syms) and len(fixed) == 1:
        return syms[0][0]
    return None


def check(name, rows, clues, expected):
    colours = parse(rows)
    states = states_of(colours)
    size = {cell: len(s) for s in states for cell in s}
    capitols = [c for c in map(capitol, states) if c is not None]

    dist = {c: 0 for c in capitols}
    heap = [(0, c) for c in capitols]
    heapq.heapify(heap)
    while heap:
        d, (r, c) = heapq.heappop(heap)
        if d > dist[(r, c)]:
            continue
        for n in ((r + 1, c), (r - 1, c), (r, c + 1), (r, c - 1)):
            if n in colours:
                nd = d + min(size[(r, c)], size[n])
                if nd < dist.get(n, 10**9):
                    dist[n] = nd
                    heapq.heappush(heap, (nd, n))

    bad = {k: (v, dist[k]) for k, v in clues.items() if dist[k] != v}
    assert not bad, f"{name}: clue mismatches {bad}"
    side = max(r for r, _ in colours) + 1
    row_sums = [sum(size[(r, c)] for c in range(side)) for r in range(side)]
    answer = sum(s * s for s in row_sums)
    print(f"{name}: {len(states)} states, {len(capitols)} capitols, {len(clues)}/{len(clues)} clues ok")
    print(f"  state sizes: {sorted(len(s) for s in states)}")
    print(f"  row sums: {row_sums}")
    print(f"  answer: {answer}")
    assert answer == expected, f"{name}: expected {expected}"


EXAMPLE = ["P B B R R", "P B B G R", "P P G G R", "R P P P R", "R R R R R"]
EXAMPLE_CLUES = {(0, 0): 12, (0, 4): 17, (1, 0): 8, (1, 2): 6, (1, 3): 3, (3, 1): 7,
                 (3, 2): 6, (3, 4): 10, (4, 0): 25, (4, 4): 0}

# Division published on the official solution page, one letter per colour.
GRID = [
    "G G G G B R Y Y Y Y Y",
    "B B B B B R R R G G G",
    "B P P P P R P R G B G",
    "G P P P P R R R G B G",
    "G G P P P P Y R P B B",
    "G P P P P P Y Y P B R",
    "G G G B P P P Y Y B B",
    "R R G B Y P P P P P B",
    "R G G Y Y P P P P B B",
    "R R G R Y G P P P P B",
    "R R R R G G P P P P B",
]
CLUE_ROWS = {
    0: {1: 8, 2: 4, 5: 11, 9: 5}, 1: {7: 11}, 2: {2: 7, 5: 1, 10: 14},
    3: {0: 28, 3: 51, 6: 1, 8: 6}, 4: {1: 22, 8: 4, 10: 1}, 5: {3: 15, 5: 10, 7: 0},
    6: {0: 11, 2: 11, 9: 9}, 7: {2: 17, 4: 14, 7: 10, 10: 13}, 8: {0: 30, 5: 6, 8: 45},
    9: {3: 10}, 10: {1: 26, 5: 0, 8: 77, 9: 61},
}
CLUES = {(r, c): v for r, row in CLUE_ROWS.items() for c, v in row.items()}

if __name__ == "__main__":
    check("example", EXAMPLE, EXAMPLE_CLUES, 8045)
    check("puzzle", GRID, CLUES, 408951)
    print("all checks passed")
