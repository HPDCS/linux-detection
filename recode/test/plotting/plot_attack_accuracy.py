#!/bin/python3
import os
import sys
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt   


def plot_sc_accuracy(name, df):

    plt.figure(figsize=(7, 4.75))

    sns.set(style="whitegrid", color_codes=True, font="Verdana", font_scale=1.2)
    
    # palette = sns.color_palette("bright")
    # sns.palplot(palette)
    
    sns.lineplot(data=df, marker='o', linewidth=2)
    
    sns.set(style="whitegrid", color_codes=True, font="Verdana", font_scale=1.1)
    
    # plt.legend(title='Smoker', loc='upper left', labels=['Hell Yeh', 'Nah Bruh'])
    plt.ylabel("Attack's extracted bytes (% of 256 bytes)")
    plt.xlabel("Max delay between two victim's reads (μsec)")
    plt.suptitle("Attack detection on " + name, fontsize=15, fontweight='bold')

    plt.legend(frameon=True, fontsize=14)

    plt.savefig("extract_" + name + ".pdf", bbox_inches='tight')
    # plt.show()
    plt.clf()


if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("Required a directoy name")
        exit(0)

    shift = list(range(0, 10))
    shift = list(map(lambda x: str(int((65536 >> x) / 1000)) + "K" if (65536 >> x) > 1000 else str(65536 >> x), shift))

    for (dirpath, dirnames, filenames) in os.walk(sys.argv[1]):

        print(str(dirpath) + "; " + str(dirnames) + "; " + str(filenames))

        processed = {}

        for fileName in filenames:
            file = open(dirpath + "/" + fileName, "r")
            lines = file.readlines()
            file.close()

            parsed = {}

            for line in lines:
                if "name" in line:
                    name = line.strip().split(" ")[1]
                elif "plain" in line:
                    type = "plain"
                    parsed["plain"] = []
                elif "gamma" in line:
                    type = line.strip().split(" ")[1]
                    parsed[type] = []
                elif "avg" in line:
                    line = line[line.index("avg:") + len("avg:"):]
                    line = line[:line.index("]")]
                    
                    if type == "plain":
                        line = str(float(line) * 4)
                    parsed[type].append(float(line))

            processed[name] = parsed

        for p in processed:
            print("Processing " + p)
            df = pd.DataFrame(columns=["γ = " + str(k) for k in processed[p]].insert(0, "shift"))
            df["shift"] = shift

            for k in processed[p]:
                if k == "plain":
                    df[k] = processed[p][k]
                else:
                    df["γ = " + str(k)] = processed[p][k]

            for k in df:
                if k == "plain" or k == "shift":
                    continue
                df[k] /= df["plain"]

            df.pop("plain")
            df = df.set_index("shift")
            print(df)
            plot_sc_accuracy(p, df)