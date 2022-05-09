#!/bin/python3
import sys
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt   
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay


def pruneAfter(str, key):
    return str.substr(str.index(key) + len(key)).strip()


class Detected:
    def __init__(self, pid, name, score, ticks, allS1, allP4, metrics=[]):
        self.pid = pid
        self.name = name
        self.score = int(score)
        self.ticks = int(ticks)
        self.allS1 = int(allS1)
        self.allP4 = int(allP4)
        self.metris = []
        for m in metrics:
            self.metris.append(int(m))

    def isComplete(self):
        return self.tests != -1 and self.eRunTime != -1 and self.eInstallTime != -1 and self.downloadSize != -1 and self.environmentSize != -1

    def __str__(self):
        return self.name + ': ' + str(self.score) + ' / ' + str(self.ticks)

    def isS1(self):
        return self.allS1 > self.allP4

    def isP4(self):
        return not self.isS1()


def countS1(dts, ths=0):
    return sum(map(lambda d: (ths == 0 or d.score >= ths) and d.isS1(), dts))


def countP4(dts, ths=0):
    return sum(map(lambda d: (ths == 0 or d.score >= ths) and d.isP4(), dts))


def countOver(dts, ths=1):
    return sum(map(lambda d: d.score >= ths, dts))


def plot_confusion_matrix(df_confusion, title='Confusion matrix', cmap=plt.cm.gray_r):
    plt.matshow(df_confusion, cmap=cmap) # imshow
    #plt.title(title)
    plt.colorbar()
    tick_marks = np.arange(len(df_confusion.columns))
    plt.xticks(tick_marks, df_confusion.columns, rotation=45)
    plt.yticks(tick_marks, df_confusion.index)
    #plt.tight_layout()
    plt.ylabel(df_confusion.index.name)
    plt.xlabel(df_confusion.columns.name)
    plt.show()



if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("Required a file name")
        exit(0)

    file = open(sys.argv[1], "r")
    lines = file.readlines()
    file.close()

    detList = []
    bench = "None"
    watching = False
    all_bench = 0
    name = "XXX"

    if "name" in lines[0]:
        name = lines[0].strip().split(" ")[1]

    for _line in lines:
        if watching:
            # Remove time
            line = " ".join(_line.split(" ")[2:]).strip()
            line = line.split(":")
            try:
                pid = int(line[0])
                if len(line) > 2:
                    line[1] += line[2]
                    line = line[:2]
                line = line[1].strip().split("\t")
                d = Detected(pid, line[0], line[1], line[2], line[3], line[4])
                detList.append(d)
                # print(d)
                if len(line) > 5:
                    p1 = line[5]
                    p2 = line[6]
                    p3 = line[7]
                    p4 = line[8]
                    p5 = line[9]
                
                """ Remember - p3 is not used to identify """
                """ Remember - p3 may be incorrect in case of few PMCs """
                all_bench += 1

                continue
            except IndexError:
                print(_line)
                print(line)
            except ValueError:
                watching = False
        
        # Start new bench
        if "**" in _line:
            bench = _line.split(" ")[-1]
            all_bench += 1
        #  Parsing detections
        elif "* Watched" in _line:
            watching = True
        elif "Test completed:" in _line:
            nr_bench = _line.strip().split(" ")[-1]
            print("Completed: " + nr_bench)
        elif "Detected:" in _line:
            nr_attack = _line.split(" ")[-1]
            print("Detected: " + nr_attack)

    print("by S1: " + str(countS1(detList)))
    print("by P4: " + str(countP4(detList)))

    print()
    print("Under threshold:")

    f, axes = plt.subplots(1, 5, figsize=(18, 3.25), sharey='row')

    for i, ths in enumerate([1, 2, 5, 10, 20]):
        print(str(ths) + " < : " + str(countOver(detList, ths)))

        """ Print the confusion matrix """
        """ POSITIVES - ATTACKS - byS1 - byP4 """
        y_actu = [all_bench]  # , 0, 0, 0]
        y_pred = [all_bench]  # - countOver(detList), countOver(detList), countS1(detList), countP4(detList)]

        print(y_actu)
        print(y_pred)

        ls1 = ["s1"] * 60
        lp4 = ["p4"] * 30
        lok = ["ok"] * all_bench

        ps1 = ["s1"] * 60
        pp4 = ["p4"] * 30
        pok = ["ok"] * (all_bench - countOver(detList, ths))
        xs1 = ["s1"] * countS1(detList, ths)
        xp4 = ["p4"] * countP4(detList, ths)

        trueSet = ls1 + lp4 + lok
        predSet = ps1 + pp4 + pok + xs1 + xp4

        sns.set(font_scale=1.5)

        cm = confusion_matrix(trueSet, predSet, normalize='true')
        disp = ConfusionMatrixDisplay(confusion_matrix=cm,
                                    display_labels=["OK", "P4", "S1"])

        sns.set(font_scale=1)

        # disp.plot(ax=axes[i], xticks_rotation=45, cmap=plt.cm.Blues)
        disp.plot(ax=axes[i], cmap=plt.cm.Blues)
        disp.ax_.set_title("γ = " + str(ths))
        disp.im_.colorbar.remove()
        disp.ax_.set_xlabel('')
        if i != 0:
            disp.ax_.set_ylabel('')
        else:
            disp.ax_.set_ylabel('True label', fontsize=13)


        # disp.plot(cmap=plt.cm.Blues)
        # plt.title("Processes classification with gamma = " + str(ths))
        # plt.show()


    # f.suptitle("Processes classification on " + name)
    f.text(0.405, 0.04, 'Predicted label', ha='left', fontsize=13)
    f.text(0.360, 0.92, 'Detection on ' + name, ha='left', fontsize=14, fontweight='bold')
    plt.subplots_adjust(wspace=0.25, hspace=0)
    f.colorbar(disp.im_, ax=axes)
    plt.savefig("detect_" + name + ".pdf", bbox_inches='tight')
    plt.show()