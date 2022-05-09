#!/bin/python3
from preliminary.extract_bench import *
import re
import sys
import logging as log
from datetime import datetime
from threading import Timer

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *

RECODE_PATH = TOOLS_PATH + "/recode.py"


def exec(cmdList, prog=RECODE_PATH, type=Pipe.ERR):
    cmdList.insert(0, prog)
    out = cmd(" ".join(cmdList), type=type, sh=True)
    if out and out != "":
        print(out)


def clearText(text):
    return re.sub(r"\x1b(\[.*?[@-~]|\].*?(\x07|\x1b\\))", "", text)


CMD_TEST = "FORCE_TIMES_TO_RUN=1 phoronix-test-suite batch-run "


def sendInput(arg):
    print("FORCE Sent Input")
    arg.write(bytes(str(1) + "\n", "utf-8"))
    arg.flush()


def readPTSOptions(bench):
    process = icmd([CMD_TEST + bench], sh=True)

    options = []

    while True:
        timer = Timer(2.0, sendInput, [process.stdin])
        timer.start()
        output = clearText(process.stdout.readline().decode("utf-8"))
        timer.cancel()

        if output == "" and process.poll() is not None:
            break
        if output:
            print(output)
            if "Test All Options" in output:
                maxValue = int(output.strip().split(":")[0])
                options.append(maxValue)
            elif "** Multiple items can be selected, delimit by a comma. **" in output:
                process.stdin.write(bytes(str(maxValue) + "\n", "utf-8"))
                process.stdin.flush()
            elif "System Information" in output:
                process.terminate()
                break

    if len(options) == 0:
        return [1]
    return options


def runPTSBenchmark(bench, options):
    process = icmd([CMD_TEST + bench], sh=True)

    curOption = 0

    while True:
        output = clearText(process.stdout.readline().decode("utf-8"))
        if output == "" and process.poll() is not None:
            break
        if output:
            print(output)
            if "** Multiple items can be selected, delimit by a comma. **" in output:
                process.stdin.write(bytes(str(options[curOption]) + "\n", "utf-8"))
                process.stdin.flush()
                curOption += 1

    return options


def runBenchmark(bench, maxRuns=3):
    options = readPTSOptions(bench)
    log.info("Options: " + str(options))

    print(options)

    curOptions = []

    for option in options:
        curOptions.append(1)

    run = 0
    detectedCounter = 0
    exist = False

    while run < maxRuns:
        log.info("Run with opt: " + str(curOptions))
        cmd(
            'echo "' + bench + "_" + str(curOptions) + '" > /proc/recode/statistics',
            sh=True,
        )
        log.info("** Registered to ReCode statistics: " + bench + "_" + str(curOptions))
        runPTSBenchmark(bench, curOptions)
        detected = int(cmd("cat /proc/recode/statistics", type=Pipe.OUT, sh=True))
        log.info("Run finished with DETECTED: " + str(detected))

        """ Read SysLog """
        file = open("/var/log/syslog", "r")
        lines = file.readlines()
        file.close()

        lIdx = 0

        for i in range(len(lines)):
            if "*** Running BENCH:" in lines[i]:
                lIdx = i

        watched = {}
        detected = []

        lines = lines[lIdx:]
        for line in lines:
            if "[++]" in line:
                line = line[line.index("[++]") :].replace("[++]", "").strip().split(" ")
                key = line[1]
                if key not in watched or watched[key][2] < line[2]:
                    line.pop(1)
                    watched[key] = line
            elif "[!!]" in line:
                detectedCounter += 1
                detected.append(line[line.index("[!!]") :].replace("[!!]", "").strip())

        log.info("* Watched")
        for key in watched:
            log.info(key + ": " + "\t".join(watched[key]))

        if len(detected) > 0:
            log.info("* Detected")
            for elem in detected:
                log.info(elem)

        run += 1

        for i in reversed(range(len(curOptions))):
            if curOptions[i] < options[i]:
                curOptions[i] += 1
                exist = True
                break

        if not exist:
            break

    return run, detectedCounter


BLACKLIST = [
    "pts/dcraw",
    "pts/stream",
    "pts/tscp",
    "pts/render-bench",
    "pts/m-queens",
    "pts/scimark2",
    "pts/fio",
    "pts/sqlite",
    "pts/jxrendermark",
    "pts/aio-stress",
    "pts/build-mplayer",
    "pts/crafty",
    "pts/t-test1",
    "pts/core-latency",
    "pts/n-queens",
    "pts/ipc-benchmark",
    "pts/cpuminer-opt",
    "pts/ctx-clock",
    "pts/fhourstones",
    "pts/webp",
    # Other benchmarks
    "pts/apache",
    "pts/sqlite-speedtest",
    "pts/osbench",
    "pts/hackbench"
    "pts/compilebench"
    "pts/sockperf"
]


if __name__ == "__main__":

    """Load ReCode and tune parameters"""

    exec(["module", "-u"])

    exec(["module", "-cc", "SECURITY"])

    exec(["module", "-l"])

    exec(["config", "-f", "20"])

    """ Tuning FR """
    exec("config -s tuning -tt FR".split())

    prog = TOOLS_PATH + "/attacks/flush_flush/fr/spy"
    args = '"/usr/lib/x86_64-linux-gnu/libgtk-x11-2.0.so.0.2400.33 0x279b80"'

    execStr = ("profiler -c 1 -t 3 -x " + prog).split()
    execStr.append("-a")
    execStr.append(args)
    exec(execStr)

    exec("config -s tuning -tt XL".split())

    prog = TOOLS_PATH + "/attacks/xlate/obj/aes-xp"
    args = TOOLS_PATH + "/attacks/xlate/openssl-1.0.1e/libcrypto.so.1.0.0"

    execStr = ("profiler -c 1 -t 3 -x " + prog).split()
    execStr.append("-a")
    execStr.append(args)
    exec(execStr)

    print("Thresholds:")
    exec(["/proc/recode/thresholds"], "cat", Pipe.OUT)

    """ ******************* """
    """ Start Testing Phase """
    """ ******************* """

    """ Install Benchmarks """
    installedBenchmarks = getPTSListInstalledTests()

    """ Start testing """
    now = datetime.now()
    dt_string = now.strftime("%d_%m_%Y_%H_%M_%S")

    log.basicConfig(
        filename=dt_string + ".log",
        format="%(asctime)s %(message)s",
        encoding="utf-8",
        level=log.DEBUG,
    )
    log.info("Starting ReCode and Tuning")

    """ Enable ReCode """
    exec("config -s system".split())

    detectedCounter = 0
    benchmarkCounter = 0

    RESUME = ""
    log.info("Start with benchmarks")

    offSet = 0
    if (RESUME != ""):
        try:
            offSet = installedBenchmarks.index(RESUME)
            print("Start from index: " + str(offSet))
        except ValueError:
            print("Cannot find RESUME.. SKIP")

    # for bench in installedBenchmarks[offSet:]:
    for bench in ["pts/amg", "pts/botan", "pts/chia-vdf", "pts/compress-pbzip2", "pts/glibc-bench", "pts/john-the-ripper", "pts/liquid-dsp", "pts/neatbench", "pts/qmcpack", "pts/scikit-learn", "pts/smhasher", "pts/srsran", "pts/gegl", "pts/gimp"]:
        if bench in BLACKLIST:
            print(bench + " is in NOT_ALLOWED_LIST... SKIP")
            continue
        log.info("Processing: " + bench)
        run, det = runBenchmark(bench, 3)
        benchmarkCounter += run
        detectedCounter += det

    log.info("Test completed: " + str(benchmarkCounter))
    log.info("Detected: " + str(detectedCounter))

    print("Test completed: " + str(benchmarkCounter))
