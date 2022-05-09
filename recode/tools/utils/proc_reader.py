import time

PATH_PROC_RECODE = "/proc/recode/cpus/"


class ProcCpuReader:

    def __init__(self, cpuId="cpu0"):
        self.evtList = []
        self.dictLines = {}
        self.pFile = open(PATH_PROC_RECODE + cpuId, "r")

    def __read_cpu(self):
        """ Look for Event Labels """
        if (len(self.evtList) == 0):
            line = self.pFile.readline()
            if (line.startswith("#")):
                self.evtList.extend([i for i in line[1:].strip().split(",")])
                for k in self.evtList:
                    self.dictLines[k] = []
            else:
                print("Cannot find Event List, line:" + line)
                return 0

        """ Get the raw line and simultaneously build the dict """
        readLines = self.pFile.readlines()
        for line in readLines:
            for i, w in enumerate(line.strip().split(",")):
                self.dictLines[self.evtList[i]].append(int(w))

        return len(readLines)

    def try_read(self):
        if (self.__read_cpu() == 0):
            return None
        return self.dictLines

    def read(self):
        while (self.__read_cpu() == 0):
            time.sleep(1)

        return self.dictLines

    def close(self):
        self.pFile.close()


# TODO - Sample header is hard coded, but should be taken from recode
SAMPLE_HEADER = ["PID", "TRACKED", "KTHREAD", "TSC",
                 "TSC_CYCLES", "CORE_CYCLES", "CORE_CYCLES_TSC", "MASK"]

MASK_HEADER = {0:  ["BB", "BS", "RE", "FB"],
               1:  ["BB", "BS", "RE", "FB", "MB", "CB"],
               2:  ["L1B", "L2B", "L3B", "DRAMB"],
               3:  ["BB", "BS", "RE", "FB", "MB", "CB", "L1B", "L2B", "L3B", "DRAMB"]}

MASK_HEADER_FULL = ["BB", "BS", "RE", "FB",
                    "MB", "CB", "L1B", "L2B", "L3B", "DRAMB"]


class ProcCpuReaderTMA:

    def __init__(self, cpuId="cpu0"):
        self.evtList = {}
        self.dictLines = {}
        self.pFile = open(PATH_PROC_RECODE + cpuId, "r")

    def __read_cpu(self):
        """ Create Dict from Header """
        for k in SAMPLE_HEADER:
            self.dictLines[k] = []
        # Create a super set of columns
        for k in MASK_HEADER_FULL:
            self.dictLines[k] = []

        offset = len(SAMPLE_HEADER)

        """ Get the raw line and simultaneously build the dict """
        readLines = self.pFile.readlines()
        for line in readLines:
            values = line.strip().split(",")
            mask = int(values[offset - 1])

            # Define fixed part
            for i, w in enumerate(values[0: offset]):
                self.dictLines[SAMPLE_HEADER[i]].append(int(w))

            k = 0
            for i, w in enumerate(MASK_HEADER_FULL):
                if w in MASK_HEADER[mask]:
                    self.dictLines[MASK_HEADER_FULL[i]].append(
                        int(values[offset + k]))
                    k += 1
                else:
                    self.dictLines[MASK_HEADER_FULL[i]].append(-1)

        return len(readLines)

    def try_read(self):
        if (self.__read_cpu() == 0):
            return None
        return self.dictLines

    def read(self):
        while (self.__read_cpu() == 0):
            time.sleep(1)

        return self.dictLines

    def close(self):
        self.pFile.close()
