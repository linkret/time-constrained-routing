from pulp import *
from tqdm import trange


def main():
    for path in [
        'instances/inst1.TXT',
        'instances/inst2.TXT',
        'instances/inst3.TXT',
        'instances/inst4.TXT',
        'instances/inst5.TXT',
        'instances/inst6.TXT',
    ]:
        l = open(path).readlines()

        if l[-1] == '':
            l = l[:-1]

        n, q = map(int, l[2].split())
        raw = [list(map(int, x.split())) for x in l[7:]]

        d = [
            x[3] for x in raw
        ]

        lo, hi = 1, n
        while lo < hi:
            v_num = (lo + hi) // 2
            model = LpProblem("Binning", LpMinimize)

            # use highs, silent
            model.solver = HiGHS_CMD(msg=False)

            # variables
            x = LpVariable.dicts(
                "x", (range(len(raw)), range(v_num)), 0, 1, LpBinary)

            # customer in one vehicle
            for i in range(len(raw)):
                model += lpSum([x[i][j] for j in range(v_num)]) == 1

            # capacity
            for i in range(v_num):
                model += lpSum([d[j] * x[j][i] for j in range(len(raw))]) <= q

            model.solve()

            # for i in range(v_num):
            #     print("Vehicle ", i)
            #     for j in range(n):
            #         if x[j][i].value() == 1:
            #             print(" Customer ", j, " demand ", d[j])

            if model.status == 1:
                hi = v_num
            else:
                lo = v_num + 1

        print(path, lo)


if __name__ == '__main__':
    main()
