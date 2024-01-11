from pulp import *
from tqdm import trange
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt
import itertools

res = {0: [82, 87, 152, 200, 299, 321, 335, 371, 389, 397], 1: [39, 177, 181, 190, 210, 288, 295, 297, 332, 355, 391, 400], 2: [24, 25, 30, 73, 134, 145, 199, 249, 264, 281, 287, 320, 377], 3: [15, 33, 86, 166, 173, 179, 243, 285, 327, 334, 352], 4: [12, 76, 93, 144, 194, 202, 214, 256, 283, 296, 315, 343, 363], 5: [27, 51, 71, 139, 198, 263, 265, 292, 303, 354], 6: [66, 81, 83, 88, 171, 212, 248, 282, 368, 378], 7: [75, 121, 136, 150, 213, 217, 245, 318, 383], 8: [2, 3, 47, 77, 90, 108, 127, 133, 159, 196, 242, 344, 348, 360], 9: [1, 4, 89, 109, 110, 111, 114, 195, 271, 279, 345, 347, 364], 10: [5, 91, 151, 307, 324, 330, 373, 385], 11: [57, 116, 157, 225, 234, 246, 280, 286, 289, 305, 338, 350, 356], 12: [7, 17, 41, 59, 78, 84, 104, 167, 232, 322, 333], 13: [43, 50, 106, 149, 158, 189, 205, 252, 386], 14: [8, 46, 55, 103, 120, 131, 178, 203, 222, 270, 359, 369, 376], 15: [52, 53, 65, 118, 138, 141, 154, 168, 170, 180, 207, 266, 370], 16: [28, 35, 36, 58, 182, 186, 250, 273, 331, 357, 398], 17: [9, 45, 94, 99, 100, 135, 241, 278, 298,
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    328, 381], 18: [18, 126, 208, 277, 284, 301, 308, 319, 337], 19: [21, 74, 112, 132, 176, 197, 218, 223, 255, 274, 326, 351, 390], 20: [13, 44, 63, 125, 142, 155, 187, 259, 269, 304, 306, 323], 21: [20, 26, 105, 119, 123, 163, 226, 261, 310], 22: [11, 129, 137, 174, 185, 268, 294, 302, 339, 341, 382, 394], 23: [31, 67, 95, 101, 233, 237, 258, 260, 290, 346, 362, 366], 24: [10, 80, 206, 235, 236, 239, 291, 375, 380], 25: [92, 115, 257, 267, 272, 300, 312, 340, 367, 396], 26: [48, 54, 70, 102, 175, 192, 215, 293, 336, 365, 387, 395], 27: [14, 22, 23, 32, 60, 98, 143, 193, 201, 262, 329], 28: [19, 42, 61, 68, 146, 147, 156, 160, 228, 230, 342, 379, 388], 29: [34, 49, 56, 107, 117, 172, 204, 219, 276, 317, 374], 30: [16, 128, 188, 216, 227, 244, 247, 309, 353], 31: [97, 124, 165, 191, 224, 231, 314, 316, 384], 32: [72, 85, 140, 161, 164, 169, 184, 211, 238, 313, 349, 361, 393], 33: [37, 153, 209, 221, 240, 254, 311, 325, 399], 34: [29, 64, 79, 96, 113, 122, 183, 220, 229], 35: [6, 38, 40, 62, 69, 130, 148, 251, 253, 275, 358, 372, 392]}


def tsp(best_result, xs, ys, a, b, v_num):

    d_matrix = [
        [
            ((xs[i] - xs[j])**2 + (ys[i] - ys[j])**2)**0.5
            for i in range(len(xs))
        ]
        for j in range(len(xs))]

    for route in best_result.values():
        model = LpProblem("TSP", LpMinimize)

        # use highs, silent
        model.solver = getSolver('HiGHS_CMD')
        x = LpVariable.dicts(
            "x", (range(len(route)), range(len(route))), 0, 1, LpBinary)
        s = LpVariable.dicts(
            "s", (range(len(route))), 0, None, LpContinuous)

        # one per row
        for i in range(len(route)):
            model += lpSum([x[i][j] for j in range(len(route))]) == 1

        # one per column
        for j in range(len(route)):
            model += lpSum([x[i][j] for i in range(len(route))]) == 1


def main():

    for path in [
        'instances/inst2.TXT',

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

        tsp(res, xs, ys, a, b, v_num)
        exit()

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

        tsp(best_result, xs, ys, a, b, v_num)

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
