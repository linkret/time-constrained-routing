from pulp import *
from tqdm import trange
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt
import itertools


def main():

    for path in [
        'instances/inst4.TXT',

    ]:
        l = open(path).readlines()

        if l[-1] == '':
            l = l[:-1]

        n, q = map(int, l[2].split())
        raw = [list(map(int, x.split())) for x in l[7:]]

        xs = [x[1] for x in raw]
        ys = [x[2] for x in raw]
        a = [x[4] for x in raw]
        b = [x[5] for x in raw]

        d = [
            x[3] for x in raw
        ]

        v_num = 36

        model = LpProblem("Binning", LpMinimize)

        # use highs, silent
        model.solver = getSolver('HiGHS_CMD', timeLimit=20)

        # variables
        x = LpVariable.dicts(
            "x", (range(1, len(raw)), range(v_num)), 0, 1, LpBinary)

        # customer in one vehicle
        for i in range(1, len(raw)):
            model += lpSum([x[i][j] for j in range(v_num)]) == 1

        # capacity
        for i in range(v_num):
            model += lpSum([d[j] * x[j][i] for j in range(1, len(raw))]) <= q

        # objective
        linExpr = LpAffineExpression()

        k_means = KMeans(n_clusters=v_num, n_init="auto")
        k_means.fit([[xs[i], ys[i]] for i in range(1, len(raw))])

        for i in range(v_num):
            for j in range(1, len(raw)):
                linExpr += (
                    (xs[j] - k_means.cluster_centers_[i][0])**2 +
                    (ys[j] - k_means.cluster_centers_[i][1])**2
                ) * x[j][i]

        model += linExpr

        model.solve()

        # for i in range(v_num):
        #     print("Vehicle ", i)
        #     for j in range(n):
        #         if x[j][i].value() == 1:
        #             print(" Customer ", j, " demand ", d[j])

        # plot the result

        best_result = {
            i: [j for j in range(1, len(raw)) if x[j][i].value() == 1]
            for i in range(v_num)
        }

        # solve tsp for each vehicle

        for i in range(v_num):
            # plot

            plt.scatter(
                [xs[j] for j in best_result[i]],
                [ys[j] for j in best_result[i]],
                label=f"vehicle {i}",
            )

        plt.legend()
        plt.show()


if __name__ == '__main__':
    main()
