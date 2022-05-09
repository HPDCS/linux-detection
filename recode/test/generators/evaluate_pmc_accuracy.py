#!/bin/python3
# import extract_bench as eb
from preliminary.extract_bench import *
import sys
import numpy as np

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *


PERF_CMD = "perf stat -e L1-dcache-loads:u,L1-dcache-loads-misses:u"

PERF_SINGLE_CMD = "perf stat -e r10b0:u,r07f1:u,r0108:u"

CACHEGRIND_CMD = "valgrind --tool=cachegrind"

BENCHMARK = "sysbench cpu --cpu-max-prime=100000 --threads=1 --events=1 run"


def cachegrind_extract(line, elem, postfix):
    line = line[line.index(elem) + len(elem):]
    line = line.replace("(", "+").replace(")", "+").strip().split("+")
    for val in line:
        if postfix in val:
            return val.replace(postfix, "").strip().replace(",", "")

    return ""


def perf_extract(line):
    return list(filter(lambda a: a != "", line.split(" ")))[0].replace(".", "")


def exec_cachegrind():
    loads = -1
    lmiss = -1

    out, err, ret = cmd([CACHEGRIND_CMD + " " + BENCHMARK], sh=True)

    for line in err.split("\n"):
        if "D   refs:" in line:
            loads = cachegrind_extract(line, "D   refs:", "rd")
        elif "D1  misses:" in line:
            lmiss = cachegrind_extract(line, "D1  misses:", "rd")
        # elif "D1  miss rate:" in line:
        #     lrate = cachegrind_extract(line, "D1  miss rate:", "%")

    return [float(loads), float(lmiss), float(lmiss) / float(loads)]


def exec_perf():
    loads = -1
    lmiss = -1

    out, err, ret = cmd([PERF_CMD + " " + BENCHMARK], sh=True)

    for line in err.split("\n"):
        if "L1-dcache-loads " in line:
            loads = perf_extract(line)
        elif "L1-dcache-loads-misses " in line:
            lmiss = perf_extract(line)

    return [float(loads), float(lmiss), float(lmiss) / float(loads)]


def exec_perf_single():
    l3loads = -1
    l2lines = -1
    tlbmiss = -1

    out, err, ret = cmd([PERF_SINGLE_CMD + " " + BENCHMARK], sh=True)

    for line in err.split("\n"):
        if "r10b0" in line:
            l3loads = perf_extract(line)
        elif "r07f1" in line:
            l2lines = perf_extract(line)
        elif "r0108" in line:
            tlbmiss = perf_extract(line)

    return [float(l3loads), float(l2lines), float(tlbmiss)]


if __name__ == "__main__":

    RUNS = 32
    run = 0

    perf = [[], [], []]
    cgrind = [[], [], []]

    while(run < RUNS):
        ll = exec_perf()
        perf[0].append(ll[0])
        perf[1].append(ll[1])
        perf[2].append(ll[2])

        ll = exec_cachegrind()
        cgrind[0].append(ll[0])
        cgrind[1].append(ll[1])
        cgrind[2].append(ll[2])

        run += 1

    cmd(["rm cachegrind.out*"], sh=True)

    perf_single = [[], [], []]

    run = 0
    while(run < RUNS):
        ll = exec_perf_single()
        perf_single[0].append(ll[0])
        perf_single[1].append(ll[1])
        perf_single[2].append(ll[2])

        run += 1

    print("perf")
    print(perf)
    print("cgrind")
    print(cgrind)
    print("perf_single")
    print(perf_single)

    _var = perf
    print("Perf")
    print("Loads AVG: " + str(np.average(_var[0])))
    print("Loads VAR: " + str(((np.max(_var[0]) - np.average(_var[0])) + (
        np.min(_var[0]) - np.average(_var[0])) / 2) / np.average(_var[0])))
    print("LMiss AVG: " + str(np.average(_var[1])))
    print("LMiss VAR: " + str(((np.max(_var[1]) - np.average(_var[1])) + (np.min(_var[1]) - np.average(_var[1])) / 2) / np.average(_var[1])))

    _var = cgrind
    print("Cachegrind")
    print("Loads AVG: " + str(np.average(_var[0])))
    print("Loads VAR: " + str(((np.max(_var[0]) - np.average(_var[0])) + (
        np.min(_var[0]) - np.average(_var[0])) / 2) / np.average(_var[0])))
    print("LMiss AVG: " + str(np.average(_var[1])))
    print("LMiss VAR: " + str(((np.max(_var[1]) - np.average(_var[1])) + (np.min(_var[1]) - np.average(_var[1])) / 2) / np.average(_var[1])))

    _var = perf_single
    print("Perf Single")
    print("L3 Loads AVG: " + str(np.average(_var[0])))
    print("L3 Loads VAR: " + str(((np.max(_var[0]) - np.average(_var[0])) + (
        np.min(_var[0]) - np.average(_var[0])) / 2) / np.average(_var[0])))
    print("L2 Lines AVG: " + str(np.average(_var[1])))
    print("L2 Lines VAR: " + str(((np.max(_var[1]) - np.average(_var[1])) + (np.min(_var[1]) - np.average(_var[1])) / 2) / np.average(_var[1])))
    print("TLB Miss AVG: " + str(np.average(_var[2])))
    print("TLB Miss VAR: " + str(((np.max(_var[2]) - np.average(_var[2])) + (np.min(_var[2]) - np.average(_var[2])) / 2) / np.average(_var[2])))