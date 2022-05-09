import os
from .printer import *


PLUGIN_NAME = "autotest"
HELP_DESC = "Configure Recode to execute test with minimum effort"


RECODE_PROC_PATH = "/proc/recode"
DEFAULT_DATA_FILE = "data.json"


def setParserArguments(parser):

    plug_parser = parser.add_parser(PLUGIN_NAME, help=HELP_DESC)

    plug_parser.add_argument(
        "-x",
        "--exec",
        metavar="prog",
        type=str,
        required=False,
        help="Execute and profile a given program. Specific program arguments inline",
    )

    plug_parser.add_argument(
        "-a",
        "--args",
        type=str,
        required=False,
        help="Execute and profile a given program. Specific program arguments inline",
    )

    plug_parser.add_argument(
        "-t",
        "--timeout",
        metavar="T",
        type=int,
        required=False,
        help="Set an execution timeout",
    )

    plug_parser.add_argument(
        "-c",
        "--cpu",
        type=int,
        required=False,
        help="Bind the program execution to a specific cpu",
    )

    plug_parser.add_argument(
        "-ra",
        "--run-attack",
        type=str,
        metavar="A",
        choices=["fr1", "fr2", "fr3", "fr4", "fr5", "pp1", "pp2", "ppl1", "ppl3", "xa", "xp"],
        required=False,
        help="Execute the attack (A)",
    )


def validate_args(args):
    return args.command == PLUGIN_NAME


def action_run_attack(args):
    TARGET_PATH = "/usr/lib/x86_64-linux-gnu/libgtk-x11-2.0.so.0.2400.33"
    TARGET_ADDR = "0x279b80"

    if args is None:
        print("Nothing to test. Specify an attack.")
        return

    # if os.geteuid() != 0:
    #     answer = input("Attacks should be run as root. Proceed anyway? [y/N] ")
    #     if answer != "y":
    #         exit(0)

    # flush_flush
    if args == "fr1":
        prog = "attacks/flush_flush/fr/spy"
        args = TARGET_PATH + " " + TARGET_ADDR
    if args == "pp1":
        prog = "attacks/flush_flush/pp/spy"
        args = TARGET_PATH + " " + TARGET_ADDR

    # xlate
    if args == "fr2":
        prog = "attacks/xlate/obj/aes-fr"
        args = "attacks/xlate/openssl-1.0.1e/libcrypto.so.1.0.0"
    if args == "pp2":
        prog = "attacks/xlate/obj/aes-pp"
        args = "attacks/xlate/openssl-1.0.1e/libcrypto.so.1.0.0"
    if args == "xa":
        prog = "attacks/xlate/obj/aes-xa"
        args = "attacks/xlate/openssl-1.0.1e/libcrypto.so.1.0.0"
    if args == "xp":
        prog = "attacks/xlate/obj/aes-xp"
        args = "attacks/xlate/openssl-1.0.1e/libcrypto.so.1.0.0"

    # mastik
    if args == "fr3":
        prog = "attacks/mastik/demo/FR-1-file-access"
        args = "attacks/mastik/demo/FR-1-file-access.c"
    if args == "fr4":
        prog = "attacks/mastik/demo/FR-2-file-access"
        args = "attacks/mastik/demo/FR-1-file-access.c attacks/mastik/demo/FR-2-file-access.c"
    if args == "fr5":
        prog = "attacks/mastik/demo/FR-function-call"
        args = TARGET_PATH + " " + TARGET_ADDR
    if args == "ppl1":
        prog = "attacks/mastik/demo/L1-capture"
        args = "20000"
    if args == "ppl3":
        prog = "attacks/mastik/demo/L3-capture"
        args = "20000"

    return prog, args


def compute(args, config):
    cmdList = []

    if not validate_args(args):
        return cmdList

    if args.run_attack:
        prog, args = action_run_attack(args.run_attack)
        execStr = ("profiler -x " + prog).split()
        execStr.append("-a")
        execStr.append(args)
        return [execStr]

    cmdList.append("module -u".split())
    cmdList.append("module -c SECURITY".split())
    cmdList.append("module -l".split())
    cmdList.append("config -f 20".split())
    cmdList.append("config -s tuning -tt FR".split())

    prog, args = action_run_attack("fr1")

    execStr = ("profiler -c 1 -t 5 -x " + prog).split()
    execStr.append("-a")
    execStr.append(args)
    cmdList.append(execStr)

    cmdList.append("config -s tuning -tt XL".split())

    prog, args = action_run_attack("xp")
    execStr = ("profiler -c 1 -t 5 -x " + prog).split()
    execStr.append("-a")
    execStr.append(args)
    cmdList.append(execStr)

    cmdList.append("config -s system".split())

    # execStr = ("profiler -x " + str(args.exec)).split()
    # if args.args is not None:
    #     execStr.append("-a")
    #     execStr.append(str(args.args))

    # if args.timeout is not None:
    #     execStr.append("-t")
    #     execStr.append(str(args.timeout))

    # print("args", str(execStr))

    # cmdList.append(execStr)
    # cmdList.append("config -s off".split())

    # if args.file is not None:
    #     cmdList.append(("data -etma " + args.file).split())

    # cmdList.append(("network -l -s " + args.file                                          ).split())

    return cmdList
