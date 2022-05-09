#!/bin/python3
import re
import sys
import pickle
import os

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *


def clearText(text):
    return re.sub(r'\x1b(\[.*?[@-~]|\].*?(\x07|\x1b\\))', '', text)

# phoronix-test-suite list-all-tests


def sortBenchamrksByField(benchmarks, field):
    return(sorted(benchmarks, key=lambda x: getattr(x, field)))


def filterOutLessBenchmarksByField(benchmarks, field, maxVal):
    minIdx = 0
    maxIdx = len(benchmarks)

    benchmarks = sortBenchamrksByField(benchmarks, field)

    for i in range(len(benchmarks)):
        if getattr(benchmarks[i], field) < 0:
            minIdx = i
        elif getattr(benchmarks[i], field) > maxVal:
            maxIdx = i
            break
    
    return benchmarks[minIdx:maxIdx]


def filterOutIncludeBenchmarksByField(benchmarks, field, allowed):
    fBenchmarks = []

    for b in benchmarks:
        if getattr(b, field) in allowed:
            fBenchmarks.append(b)

    return fBenchmarks


def filterOutExcludeBenchmarksByField(benchmarks, field, allowed):
    fBenchmarks = []

    for b in benchmarks:
        if getattr(b, field) not in allowed:
            fBenchmarks.append(b)

    return fBenchmarks


class Benchmark:
    def __init__(self, name, type=None):
        self.name = name
        self.eRunTime = -1
        self.eInstallTime = -1
        self.downloadSize = -1
        self.environmentSize = -1
        self.type = type
        self.tests = 1

    def isComplete(self):
        return self.tests != -1 and self.eRunTime != -1 and self.eInstallTime != -1 and self.downloadSize != -1 and self.environmentSize != -1

    def __str__(self):
        return '[' + self.name + ' @' + self.type + ']: #' + str(self.tests) + ' (Time) r: ' + str(self.eRunTime) + ' i: ' + \
                str(self.eInstallTime) + " (Size) d: " + str(self.downloadSize) + " e: " + str(self.environmentSize)


BENCHMARKS = []
CACHE_FILE = "bench_cache.dat"


def extractBenchInfo(bench):
    rawOutput = cmd(["phoronix-test-suite info " + bench.name],
                    type=Pipe.OUT, sh=True)
    rawOutput = clearText(rawOutput).split("\n")

    for info in rawOutput:
        if "Download Size:" in info:
            bench.downloadSize = float(info.replace(
                "Download Size:", "").replace("MB", "").strip())
        elif "Environment Size:" in info:
            bench.environmentSize = float(info.replace(
                "Environment Size:", "").replace("MB", "").strip())
        elif "Estimated Run-Time:" in info:
            bench.eRunTime = float(info.replace("Estimated Run-Time:",
                                                "").replace("Seconds", "").strip())
        elif "Estimated Install Time:" in info:
            bench.eInstallTime = float(info.replace("Estimated Install Time:",
                                                    "").replace("Seconds", "").strip())
        elif "Unique Tests:" in info:
            bench.tests = float(info.replace("Unique Tests:", "").strip())
        else:
            continue

        if (bench.isComplete()):
            break


def getPTSListAllTests():
    benchmarks = []
    rawOutput = cmd(["phoronix-test-suite list-all-tests"],
                    type=Pipe.OUT, sh=True)
    rawOutput = clearText(rawOutput).split("\n")

    startIdx = rawOutput.index("Available Tests")

    rawOutput = rawOutput[startIdx + 1:]

    for bench in rawOutput:
        if (bench == ""):
            continue
        bench = bench.split()
        benchmarks.append(Benchmark(bench[0], bench[-1]))

    return benchmarks


def getPTSListInstalledTests():
    benchmarks = []
    rawOutput = cmd(["phoronix-test-suite list-installed-tests"],
                    type=Pipe.OUT, sh=True)
    rawOutput = clearText(rawOutput).split("\n")

    startIdx = 0

    for i in range(len(rawOutput)):
        if "Tests Installed" in rawOutput[i]:
            startIdx = i
            break

    rawOutput = rawOutput[startIdx + 1:]

    for bench in rawOutput:
        if (bench == ""):
            continue
        """ Remove the test version """
        name = bench.split()[0]
        name = name[:name.rfind('-')]
        benchmarks.append(name)

    return benchmarks


def getAvailableBenchmarks():
    global BENCHMARKS
    skipExtraction = False

    BENCHMARKS = getPTSListAllTests()

    if (os.path.isfile(CACHE_FILE)):
        bfile = open(CACHE_FILE, mode='rb')
        CACHED_BENCHMARKS = pickle.load(bfile)
        bfile.close()
        if (len(BENCHMARKS) == len(CACHED_BENCHMARKS)):
            skipExtraction = True
            BENCHMARKS = CACHED_BENCHMARKS

    """ We have a list of benchmarks, now we need to get the size of each of them """
    if not skipExtraction:
        print("WEB Extraction")
        for bench in BENCHMARKS:
            extractBenchInfo(bench)
            print(bench)
        bfile = open(CACHE_FILE, mode='wb')
        pickle.dump(BENCHMARKS, bfile)
        bfile.close()

    print("Compiled Benchmarks")
    filteredBenchmarks = filterOutLessBenchmarksByField(BENCHMARKS, "eRunTime", 200)
    filteredBenchmarks = filterOutLessBenchmarksByField(filteredBenchmarks, "downloadSize", 200)
    filteredBenchmarks = filterOutIncludeBenchmarksByField(filteredBenchmarks, "type", ["Processor", "System"])
    filteredBenchmarks = filterOutExcludeBenchmarksByField(filteredBenchmarks, "name", getPTSListInstalledTests())

    return filteredBenchmarks


if __name__ == "__main__":
    
    filteredBenchmarks = getAvailableBenchmarks()

    for bench in filteredBenchmarks:
        print(bench)

    print("Completed: " + str(len(filteredBenchmarks)))
