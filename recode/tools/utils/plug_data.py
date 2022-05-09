import os
import json
import pandas as pd
from .printer import *
from .proc_reader import ProcCpuReader
from .proc_reader import ProcCpuReaderTMA

PLUGIN_NAME = "data"
HELP_DESC = "Access and manipulate collected data"


RECODE_PROC_PATH = "/proc/recode"
RECODE_PROC_CPUS_PATH = RECODE_PROC_PATH + "/cpus"

DEFAULT_DATA_FILE = "data.json"

READ_ALL_CPUS = -1
READ_NO_CMD = -2


def setParserArguments(parser):

    plug_parser = parser.add_parser(PLUGIN_NAME, help=HELP_DESC)

    plug_parser.add_argument(
        "-r",
        "--read",
        metavar="X",
        nargs='?',
        type=int,
        required=False,
        default=READ_NO_CMD,
        const=READ_ALL_CPUS,
        help="Read data from all entries of /proc/recode/cpus. If X is specified, then access only cpu(X)'s data",
    )

    plug_parser.add_argument(
        "-e",
        "--extract",
        metavar="F",
        nargs='?',
        type=str,
        required=False,
        const=DEFAULT_DATA_FILE,
        help="Read and create a file (data.json) with data. You can specify the file's name by (F)",
    )

    plug_parser.add_argument(
        "-etma",
        "--extract-tma",
        metavar="F",
        nargs='?',
        type=str,
        required=False,
        const=DEFAULT_DATA_FILE,
        help="(TMA samples) Read and create a file (data.json) with data. You can specify the file's name by (F)",
    )


def __read_proc_cpu(cpuFile):
    fileName = RECODE_PROC_CPUS_PATH + "/" + cpuFile
    file = open(fileName)
    pr_info("Reading " + fileName + ":")
    data = ""

    lines = file.readlines()
    if (len(lines) == 0):
        data = "# EMPTY"
    else:
        for line in lines:
            data += line

    file.close()

    return data


def action_read(args):
    if (args == READ_NO_CMD):
        return

    if (args == READ_ALL_CPUS):
        for cpuFile in os.listdir(RECODE_PROC_CPUS_PATH):
            pr_text(__read_proc_cpu(cpuFile))
    else:
        pr_text(__read_proc_cpu("cpu" + str(args)))


def action_extract(args):
    if (args is None):
        return

    cpuList = []

    file = open(args, "w")

    for cpuFile in os.listdir(RECODE_PROC_CPUS_PATH):
        pcr = ProcCpuReader(cpuFile)

        rawDict = pcr.try_read()
        if (rawDict is not None):
            # labels = list(rawDict.keys())

            df = pd.DataFrame(rawDict)
            df = df.sort_values(by=["TIME"])
            df = df.reset_index(drop=True)

            result = df.to_json(orient="split")
            parsed = json.loads(result)

            cpuDict = {}
            cpuDict["id"] = cpuFile
            cpuDict["data"] = parsed
            cpuList.append(cpuDict)

        pcr.close()

    file.write(json.dumps(cpuList, indent=4))
    file.write("\n")
    file.close()


def action_extract_tma(args):

    print("action_extract_tma - " + str(args))

    if (args is None):
        return

    cpuList = []

    file = open(args, "w")

    for cpuFile in os.listdir(RECODE_PROC_CPUS_PATH):
        pcr = ProcCpuReaderTMA(cpuFile)

        rawDict = pcr.try_read()
        if (rawDict is not None):
            # labels = list(rawDict.keys())

            df = pd.DataFrame(rawDict)
            df = df.sort_values(by=["TSC"])
            df = df.reset_index(drop=True)

            result = df.to_json(orient="split")
            parsed = json.loads(result)

            cpuDict = {}
            cpuDict["id"] = cpuFile
            cpuDict["data"] = parsed
            cpuList.append(cpuDict)

        pcr.close()

    file.write(json.dumps(cpuList, indent=4))
    file.write("\n")
    file.close()


def validate_args(args):
    return args.command == PLUGIN_NAME


def compute(args, config):
    if not validate_args(args):
        return False

    if args.read is not None:
        action_read(args.read)

    if args.extract:
        action_extract(args.extract)

    if args.extract_tma:
        action_extract_tma(args.extract_tma)

    return True
