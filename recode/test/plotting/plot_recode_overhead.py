#!/bin/python3
import math
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt


def autolabel(rects):
    """
    Attach a text label above each bar displaying its height
    """

    font = {"size": "12", "weight": "550"}

    for i, v in enumerate(rects):
        print(v)
        log = math.log(v, 10)
        g = log > 2

        cc = len("%.1f" % float(v))
        if cc < 5:
            lb = ("%.1f" % float(v)) + "%"
        else:
            lb = str(int(v)) + "%"

        if g:
            log = 6.5 ** log
            
            ax.text(log, i - 0.2, str(lb), color="#1F0954", fontweight="bold")
        else:
            log = (10.5 if v > 0.1 else 8.5) ** log
            ax.text(log, i - 0.2, str(lb), color="#1F0954", fontweight="bold")


arDict = {}
arDict["i7-9750H"] = [
    [
        352.80,
        18.16,
        32.89,
        18.86,
        23.04,
        24.04,
        6.83,
        10.93,
        22.60,
        19.23,
        13.36,
        14.05,
    ],
    [0.62, 0.00, 0.00, 1.33, 0.71, 0.17, 0.00, 0.77, 0.68, 0.64, 0.51, 0.33],
    [0.62, 1.29, 0.00, 2.80, 1.58, 2.19, 0.00, 3.35, 1.71, 0.64, 1.88, 0.98],
    [0.62, 0.00, 0.00, 1.33, 0.71, 0.52, 0.00, 0.77, 0.68, 0.64, 1.88, 0.47],
]

arDict["i5-8250U"] = [
    [
        634.65,
        22.34,
        32.98,
        19.05,
        23.23,
        21.83,
        3.84,
        40.55,
        30.41,
        20.51,
        15.35,
        22.09,
    ],
    [
        2.97,
        0.00,
        0.08,
        0.00,
        0.13,
        1.12,
        0.00,
        0.14,
        0.00,
        0.00,
        1.02,
        1.20,
    ],
    [2.97, 0.15, 0.31, 0.00, 1.30, 3.03, 0.00, 1.48, 7.21, 0.51, 4.10, 3.63],
    [2.97, 0.00, 0.08, 0.00, 0.66, 2.31, 0.00, 1.20, 4.39, 0.00, 2.38, 1.24],
]

arDict["i7-6700HQ"] = [
    [
        506.06,
        28.64,
        41.14,
        17.89,
        26.93,
        28.51,
        12.14,
        52.73,
        33.43,
        19.47,
        14.58,
        19.65,
    ],
    [
        0.00,
        3.66,
        3.60,
        0.00,
        0.80,
        3.91,
        2.82,
        1.70,
        2.74,
        0.00,
        1.25,
        0.00,
    ],
    [0.00, 3.66, 5.80, 2.10, 2.11, 6.44, 6.89, 3.87, 6.99, 1.05, 3.19, 0.70],
    [0.00, 3.66, 4.80, 2.10, 1.89, 6.44, 6.06, 2.10, 4.56, 0.53, 2.07, 0.30],
]

arDict["i7-7600U"] = [
    [
        425.97,
        19.75,
        29.93,
        27.25,
        21.86,
        29.73,
        9.03,
        23.68,
        34.79,
        22.36,
        18.16,
        22.12,
    ],
    [0.66, 0.0, 0.0, 0.2, 0.1, 0.0, 0.27, 0.5, 0.0, 0.3, 2.29, 1.25],
    [0.75, 0.83, 0.0, 5.3, 0.28, 0.71, 3.13, 3.2, 1.93, 1.53, 3.79, 2.11],
    [0.66, 0.50, 0.00, 0.02, 0.01, 0.00, 0.52, 0.07, 0.26, 0.70, 2.87, 1.35],
]

# arDict["i7-10750H"] = [
#     [0.01, 7.11, 1.20, 1.96, 3.87, 24.83, 0.01, 0.01, 4.51, 1.67, 0.60, 2.44],
#     [0.01, 0.00, 0.01, 0.00, 2.21, 1.50, 0.01, 0.01, 1.18, 1.01, 0.51, 0.69],
#     [0.01, 5.55, 0.01, 0.33, 2.41, 1.50, 0.10, 0.01, 2.28, 3.40, 0.68, 1.40],
#     [0.01, 3.56, 0.01, 0.13, 2.21, 1.50, 0.05, 0.01, 2.06, 1.01, 0.68, 1.24]
# ]


names = [
    "ctx-clock",
    "apache",
    "selenium",
    "sqlite\nspeedtest",
    "osbench\nfile",
    "osbench\nthreads",
    "osbench\nmemory",
    "hackbench",
    "sockperf",
    "glibc-bench",
    "compileb.\ninit",
    "compileb.\nread",
]

sns.set(style="whitegrid", color_codes=True, font_scale=1)

for ar in arDict:
    mData = arDict[ar][0]
    nomData = arDict[ar][1]
    f5Data = arDict[ar][2]
    f6Data = arDict[ar][3]

    ind = np.arange(len(names))
    width = 0.3

    fig = plt.figure(figsize=(5.5, 6))
    ax = fig.add_subplot(111)

    # ax2 = ax.twinx()
    mBar = ax.barh(ind, mData, 0.9, color="#B7C9E2", edgecolor="#5B7C99")
    nomBar = ax.barh(ind - 0.3, nomData, width, color="#1FA774", edgecolor="black")
    f6Bar = ax.barh(ind, f6Data, width, color="#CEA2FD", edgecolor="black")
    f5Bar = ax.barh(ind + 0.3, f5Data, width, color="#3E82FC", edgecolor="black")


    val = np.max(f5Data)

    # for val in [1, 2, val]:
    #     ll = ax.axvline(val, 0.02, color="#D3494E", linestyle="--", lw=1, label="lancement")
    #     ax.text(
    #         val - (val * 0.09),
    #         -0.015,
    #         str(int(val)),
    #         transform=ax.get_xaxis_text1_transform(0)[0],
    #         color="#D3494E",
    #         fontweight="bold",
    #     )

    ax.set_yticks(ind + (width / 2))
    ax.set_yticklabels(names, linespacing=1)


    ax.set_xscale("log")
    # ax.set_xlim(0, 180)

    autolabel(mData)
    ax.legend((mBar[0], nomBar[0], f5Bar[0], f6Bar[0]), ('Generic', 'Monitor OFF', 'Monitor (short window)', 'Monitor (long window)'), bbox_to_anchor=(0., 1.02, 1., .102), loc='lower left', mode="expand", borderaxespad=0., ncol=2)

    plt.suptitle("Overhead evaluation on " + ar, fontsize=14, fontweight='bold')
    plt.tight_layout()
    
    print(ar)
    # plt.show()
    # plt.savefig("ovh_" + ar + ".pdf", bbox_inches='tight')
    plt.savefig("ovh_" + ar + ".pdf")

