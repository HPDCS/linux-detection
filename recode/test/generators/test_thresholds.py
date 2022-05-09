#!/bin/python3
import sys

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *

RECODE_PATH = TOOLS_PATH + "/recode.py"


def exec(cmdList, prog=RECODE_PATH, type=Pipe.ERR):
    cmdList.insert(0, prog)
    out = cmd(" ".join(cmdList), type=type, sh=True)
    if (out and out != ""):
        print(out)


if __name__ == "__main__":
    exec(["module", "-u"])

    exec(["module", "-cc", "SECURITY"])

    exec(["module", "-l"])

    for i in range(12, 31):
        print(i)
        exec(["config", "-f", str(i)])

        """ Tuning FR """
        exec("config -s tuning -tt FR".split())

        prog = TOOLS_PATH + "/attacks/flush_flush/fr/spy"
        args = "\"/usr/lib/x86_64-linux-gnu/libgtk-x11-2.0.so.0.2400.33 0x279b80\""

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