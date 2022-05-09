#!/bin/python3
import os
import sys
import time
from datetime import datetime
import logging as log

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *


RECODE_PATH = TOOLS_PATH + "/recode.py"

CMD_TEST_ENV = "FORCE_TIMES_TO_RUN=5 "
CMD_TEST_PLAIN = CMD_TEST_ENV + "phoronix-test-suite batch-run "
CMD_TEST_SUSPC = CMD_TEST_ENV + TOOLS_PATH + "/suspecter " + CMD_TEST_PLAIN

CMD_STRESS_PLAIN = TOOLS_PATH + "/attacks/flush_flush/fr/spy /usr/lib/x86_64-linux-gnu/libgtk-x11-2.0.so.0.2400.33 0x279b80"
CMD_STRESS_SUSPC = TOOLS_PATH + "/suspecter " + CMD_STRESS_PLAIN


def runPTSBenchmark(bench, opt="1", suspect=False):
    if suspect:
        process = icmd([CMD_TEST_SUSPC + bench], sh=True)
    else:
        process = icmd([CMD_TEST_PLAIN + bench], sh=True)

    while True:
        output = clearText(process.stdout.readline().decode("utf-8"))
        if output == "" and process.poll() is not None:
            break
        if output:
            print(output)
            if "** Multiple items can be selected, delimit by a comma. **" in output:
                process.stdin.write(bytes(opt + "\n", "utf-8"))
                process.stdin.flush()
            elif "Average:" in output:
                return float(output.strip().split(" ")[1])

    return 0


def exec(cmdList, prog=RECODE_PATH, type=Pipe.ERR):
    cmdList.insert(0, prog)
    out = cmd(" ".join(cmdList), type=type, sh=True)
    if (out and out != ""):
        print(out)


def execStressor(thds, log, suspect=False):
    err = ""
    ret = 0
    pid = 0
    pids = []

    print("Running " + str(thds) + " stressor")

    for i in range(thds):
        pid = os.fork()
        if pid > 0:
            pids.append(pid)
            continue

        bind = "taskset -c " + str(i) + " "
    
        if suspect:
            out, err, ret = cmd([bind + CMD_STRESS_SUSPC], sh=True)
        else:
            out, err, ret = cmd([bind + CMD_STRESS_PLAIN], sh=True)

        if ret != 0:
            if err != "":
                log.info("Stressor error on: " + str(thds))
                log.info(err)
        
        exit(0)
    
    if pid > 0:
        return pids


def killStressor(pids):
    pids = cmd(["pgrep -f spy"], type=Pipe.OUT, sh=True).split("\n")
    pids = list(filter(lambda a: a != "", pids))

    while len(pids) > 1:
        for pid in pids:
            cmd(["kill -9 " + pid], sh=True)
        time.sleep(1)
        pids = cmd(["pgrep -f spy"], type=Pipe.OUT, sh=True).split("\n")
        pids = list(filter(lambda a: a != "", pids))


def execCycle(log, type, bench, opt, suspect=False):
    log.info("Start " + type)
    for t in threads:
        pids = execStressor(t, log, True)
        RES[type][t] = runPTSBenchmark(bench, opt)
        log.info(str(t) + ": " + str(RES[type][t]))
        killStressor(pids)


if __name__ == "__main__":

    threads = []
    nr_cpus = os.cpu_count()

    if nr_cpus == 2:
        threads = [1, 2]
    elif nr_cpus == 4:
        threads = [1, 2, 4]
    elif nr_cpus == 6:
        threads = [2, 4, 6]
    else:
        threads = [nr_cpus/4, nr_cpus/2, nr_cpus]

    RES = {}
    RES["PLAIN"] = {}
    RES["TE"] = {}
    RES["ALL"] = {}
    RES["ALWAYS"] = {}

    """ Measure BASELINE - No Mitigations """

    machine = cmd(["sudo dmidecode -s system-family"], type=Pipe.OUT, sh=True).strip().replace(" ", "_")
    manufacturer = cmd(["sudo dmidecode -s system-manufacturer"], type=Pipe.OUT, sh=True).strip().replace(" ", "_")
    product = cmd(["sudo dmidecode -s system-product-name"], type=Pipe.OUT, sh=True).strip().replace(" ", "_")

    cmdline = cmd(["cat /proc/cmdline"], type=Pipe.OUT, sh=True)

    now = datetime.now()
    hms = now.strftime("%H_%M_%S")

    log.basicConfig(
        filename=machine + "_DMP_" + hms + ".log",
        format="%(asctime)s %(message)s",
        encoding="utf-8",
        level=log.DEBUG,
    )

    log.info(manufacturer)
    log.info(machine)
    log.info(product)

    OPT = "2"
    for BENCH in ["osbench", "ctx-clock"]:
        # Security kernel
        if "5.4.145-security" in cmdline:
            log.info("** SECURITY **")

            exec(["module", "-u"])

            execCycle(log, "PLAIN", BENCH, OPT)

            log.info("Load ReCode and setup")
            # Install and Load ReCode
            exec(["module", "-cc", "SECURITY"])
            exec(["module", "-l"])
            exec(["config", "-s", "idle"])

            exec(["security", "-m", "te", "no_nx"])
            execCycle(log, "TE", BENCH, OPT)

            exec(["security", "-m", "te", "llc", "no_nx"])
            execCycle(log, "ALL", BENCH, OPT)

            exec(["security", "-m", "te", "llc", "no_nx"])
            execCycle(log, "ALWAYS", BENCH, OPT, True)

            exec(["config", "-s", "off"])

        else:
            log.info("** GENERIC **")

            execCycle(log, "ALWAYS", BENCH, OPT)

        log.info("Done")

        for k in RES["PLAIN"]:
            print(str(k) + ":")
            log.info(str(k) + ":")
            for type in ["PLAIN", "TE", "ALL", "ALWAYS"]:
                if k in RES[type]:
                    print(type + ": " + str(RES[type][k]))
                    log.info(type + ": " + str(RES[type][k]))
