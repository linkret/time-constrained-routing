from pulp import *

from pprint import pprint


def main():
    l = open('instances/inst1.TXT', 'r').read().split('\n')

    if l[-1] == '':
        l = l[:-1]

    n, q = map(int, l[2].split())

    raw = [list(map(int, x.split())) for x in l[7:]]

    xs = [x[1] for x in raw] + [raw[0][1]]
    ys = [x[2] for x in raw] + [raw[0][2]]

    t = {
        i: {
            j: ((xs[i] - xs[j]) ** 2 + (ys[i] - ys[j]) ** 2) ** 0.5
            for j in range(len(raw) + 1)
        } for i in range(len(raw) + 1)
    }

    # vehicles
    V = list(range(1, n + 1))

    # all vertices 0 is start depot, len(a) is end depot
    N = list(range(0, len(raw) + 1))

    # customers
    C = N[1:-1]

    # demand
    d = {
        i: int(raw[i][3]) for i in C
        for i in range(1, len(raw))
    }

    # time windows
    a = {
        i: int(raw[i][4]) for i in C
        for i in range(0, len(raw))
    }
    a[len(raw)] = a[0]

    b = {
        i: int(raw[i][5]) for i in C
        for i in range(0, len(raw))
    }
    b[len(raw)] = b[0]

    pprint(a)

    # pprint(d)

    model = LpProblem("Exact Time Constrained Routing", LpMinimize)

    # use highs

    model.solver = HiGHS_CMD()

    # variables

    x = LpVariable.dicts('x', (N, N, V), 0, 1, LpBinary)
    s = LpVariable.dicts('s', (N, V), 0, None, LpContinuous)

    # constraints

    # 3.2
    print("3.2")

    for i in C:
        model += lpSum(x[i][j][k] for j in N for k in V if j != i) == 1

    # 3.3
    print("3.3")

    for k in V:
        model += lpSum(d[i] * x[i][j][k] for i in C for j in N) <= q

    # 3.4
    print("3.4")

    for k in V:
        model += lpSum(x[0][j][k] for j in N) == 1

    # 3.5
    print("3.5")

    for h in C:
        for k in V:
            model += lpSum(x[i][h][k]
                           for i in N) == lpSum(x[h][j][k] for j in N)

    # 3.6
    print("3.6")

    for k in V:
        model += lpSum(x[i][len(raw)][k] for i in N) == 1

    # 3.7 (3.11 in paper)
    print("3.7")

    M = -1e9

    for i in N:
        for j in N:
            M = max(M, b[i] + t[i][j] - a[j])

    for i in N:
        for j in N:
            for k in V:
                model += s[i][k] + t[i][j] - M * \
                    (1 - x[i][j][k]) <= s[j][k]

    # 3.8
    print("3.8")

    for i in N:
        for k in V:
            model += a[i] <= s[i][k]
            model += s[i][k] <= b[i]

    # objective function 3.1

    model += lpSum(t[i][j] * x[i][j][k] for k in V for i in N for j in N)

    model.solve()

    print("Status:", LpStatus[model.status])


if __name__ == '__main__':
    main()
