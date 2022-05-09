#!/bin/python3
# import extract_bench as eb
from extract_bench import *
import sys

TOOLS_PATH = "../tools"

sys.path.insert(1, TOOLS_PATH + "/")
from utils.cmd import *


if __name__ == "__main__":

    """ Install Benchmarks """
    installedBenchmarks = getPTSListInstalledTests()

    file = open("install.sh", "w")

    file.write("#!/bin/zsh\n")
    file.write("\n")
    file.write("declare -a files=(")

    benchmarks = getAvailableBenchmarks()

    for bench in benchmarks:
        print("Installing " + bench.name)
        # if bench.name not in installedBenchmarks and bench.name not in BLACKLIST:
        file.write("\"" + bench.name + "\" ")
        # print(
        #     xcmd(["phoronix-test-suite install-dependencies " + bench.name], sh=True))

    file.write(")\n")
    file.write("\n")
    file.write("for i in \"${files[@]}\"\n")
    file.write("do\n")
    file.write("\tphoronix-test-suite install \"$i\"\n")
    file.write("done\n")

    file.close()

    print("Completed: " + str(len(benchmarks)))
