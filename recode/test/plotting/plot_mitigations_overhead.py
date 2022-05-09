#!/bin/python3
import math
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt


def autolabel(rects, pos):
    """
    Attach a text label above each bar displaying its height
    """

    font = {"size": "10", "weight": "550"}

    for i, v in enumerate(rects):
            ax.text((pos * 0.33 + i) - 0.4, v + 0.15, str(v), color="#1F0954", fontweight="bold")


names = [
    "25%",
    "50%",
    "75%",
    "100%",
]

labels = [
    "Mitigations always active",
    "Dynamic TE mitigations",
    "Dynamic T+SCE mitigations",
]

mActive = [10.10, 7.50, 6.90, 8.60]
mTe = [1.78, 1.39, 0.28, 1.54]
mTeSc = [2.30, 1.60, 0.40, 2.10]


sns.set(style="whitegrid", color_codes=True, font_scale=1)

ind = np.arange(len(names))
width = 0.25

fig = plt.figure(figsize=(5.5, 6))
ax = fig.add_subplot(111)

mActiveBar = ax.bar(ind - width - 0.05, mActive, width, color="#B7C9E2", edgecolor="#5B7C99")
mTeBar = ax.bar(ind, mTe, width, color="#CEA2FD", edgecolor="#CEA2FD")
mTeScBar = ax.bar(ind + width + 0.05, mTeSc, width, color="#3E82FC", edgecolor="#3E82FC")

# ax2 = ax.twinx()
# mBar = ax.barh(ind, mData, 0.9, color="#B7C9E2", edgecolor="#5B7C99")
# nomBar = ax.barh(ind - 0.3, nomData, width, color="#1FA774", edgecolor="black")
# f6Bar = ax.barh(ind, f6Data, width, color="#CEA2FD", edgecolor="black")
# f5Bar = ax.barh(ind + 0.3, f5Data, width, color="#3E82FC", edgecolor="black")


ax.set_xticks(ind + (width / 2))
ax.set_xticklabels(names, linespacing=1)

# ax.bar_label(mActiveBar, padding=3)
# ax.bar_label(mTeBar, padding=3)
# ax.bar_label(mTeScBar, padding=3)

autolabel(mActive, 0)
autolabel(mTe, 1)
autolabel(mTeSc, 2)
# autolabel(mTeBar)
# autolabel(mTeScBar)
ax.legend(labels=labels)
# ax.legend("Mitigations always active","Dynamic TE mitigations","Dynamic T+SCE mitigations"), bbox_to_anchor=(0., 1.02, 1., .102), loc='lower left', mode="expand", borderaxespad=0., ncol=2)

# plt.suptitle("Overhead evaluation on " + ar, fontsize=14, fontweight='bold')
plt.tight_layout()

plt.show()
# plt.savefig("ovh_" + ar + ".pdf", bbox_inches='tight')
# plt.savefig("ovh_" + ar + ".pdf")

