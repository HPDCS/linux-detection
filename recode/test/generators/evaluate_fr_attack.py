#!/bin/python3
from preliminary.extract_bench import *
import sys

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *


def scoreOpt(secret, data, sI=0, bI=0):

    if (sI >= len(secret) or bI >= len(data)):
        return 0

    try:
        eI = secret.index(data[bI], sI)

        eS = 1 + scoreOpt(secret, data, eI + 1, bI + 1)

        return eS
    except ValueError:
        return scoreOpt(secret, data, sI, bI + 1)


def runAttack(run):
    print("exec ../../fr_attack/flush " + str(run))
    out, err, ret = cmd(["taskset -c 1 ../../fr_attack/flush " + str(run)], sh=True)

    if ret != 0:
        return None

    lines = []
    try:
        lines = out.split("\n")
    except AttributeError:
        print("ERR: " + str(out))

    origin = []
    test = []
    for i in range(len(lines)):
        if "SECRET:" in lines[i]:
            origin = lines[i + 2].strip().split(" ")
            test = lines[i + 5].strip().split(" ")
            print(" *** " + lines[i + 3])
            print(" - " + lines[i + 2])
            print(" - " + lines[i + 5])
            break

    xScore = scoreOpt(origin, test)
    print("Score: " + str(xScore))
    return xScore


if __name__ == "__main__":

    MAX_TRIALS = 32

    RUNS = []

    for run in range(0, 10):
        runs = 0
        trials = 0
        minScore = 0
        maxScore = 0
        genScore = 0
        while(trials < MAX_TRIALS):
            xScore = runAttack(run)

            if xScore is not None:
                if minScore == 0 or minScore > xScore:
                    minScore = xScore

                if maxScore == 0 or maxScore < xScore:
                    maxScore = xScore

                genScore += xScore
                runs += 1

            trials += 1

        RUNS.append(str(run) + " - [avg:" + str(genScore / (runs + 0.01)) + "] [min: " + str(
            minScore) + "] [max: " + str(maxScore) + "] [runs: " + str(runs) + "]")

    for run in RUNS:
        print(run)
