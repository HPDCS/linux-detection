#!/bin/python3
import re
import sys
from datetime import datetime
import logging as log

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *


CMD_TEST_ENV = "FORCE_TIMES_TO_RUN=2 "
CMD_TEST_PLAIN = CMD_TEST_ENV + "phoronix-test-suite batch-run "


def clearText(text):
    return re.sub(r"\x1b(\[.*?[@-~]|\].*?(\x07|\x1b\\))", "", text)


def runPTSBenchmark(log, bench, opts=[""]):
    process = icmd([CMD_TEST_PLAIN + bench], sh=True)

    optI = 0

    while True:
        output = clearText(process.stdout.readline().decode("utf-8"))
        if output == "" and process.poll() is not None:
            break
        if output:
            print(output)
            log.info(output.strip())
            if "** Multiple items can be selected, delimit by a comma. **" in output:
                opt = opts[optI] if len(opts) > optI else "1"
                process.stdin.write(bytes(opt + "\n", "utf-8"))
                process.stdin.flush()
                optI += 1
            elif "Average:" in output:
                return float(output.strip().split(" ")[1])

    return 0


if __name__ == "__main__":
    machine = cmd(["sudo dmidecode -s system-family"], type=Pipe.OUT, sh=True).strip().replace(" ", "_")
    manufacturer = cmd(["sudo dmidecode -s system-manufacturer"], type=Pipe.OUT, sh=True).strip().replace(" ", "_")
    product = cmd(["sudo dmidecode -s system-product-name"], type=Pipe.OUT, sh=True).strip().replace(" ", "_")

    cmdline = cmd(["cat /proc/cmdline"], type=Pipe.OUT, sh=True)

    now = datetime.now()
    hms = now.strftime("%H_%M_%S")

    log.basicConfig(
        filename=machine + "_EVAL_" + hms + ".log",
        format="%(asctime)s %(message)s",
        encoding="utf-8",
        level=log.DEBUG,
    )

    log.info(manufacturer)
    log.info(machine)
    log.info(product)
    log.info(cmdline)

    BENCHMARKS = [
        ["ctx-clock", [""]],
        ["apache", ["1"]],
        ["selenium", ["1", "2"]],
        ["compilebench", ["1"]],
        ["compilebench", ["3"]],
        ["glibc-bench ", ["1"]],
        ["sockperf", ["1"]],
        ["hackbench", ["4", "1"]],
        ["osbench", ["1"]],
        ["osbench", ["2"]],
        ["osbench", ["5"]],
        ["sqlite-speedtest", [""]],
    ]

    for b in BENCHMARKS:
        name = b[0]
        opts = b[1]

        runPTSBenchmark(log, name, opts)
