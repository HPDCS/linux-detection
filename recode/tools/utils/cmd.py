import re
import subprocess
from enum import IntEnum


def clearText(text):
    return re.sub(r'\x1b(\[.*?[@-~]|\].*?(\x07|\x1b\\))', '', text)

class Pipe(IntEnum):
    OUT = 0
    ERR = 1
    RET = 2


def cmd(cmd_seq, type=None, sh=False, timeOut=None):
    try:
        p = subprocess.Popen(
            cmd_seq, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=sh
        )
        p.wait(timeOut)
        if type == Pipe.OUT or type == Pipe.ERR:
            return p.communicate()[type].decode("utf-8")
        elif type == Pipe.RET:
            return p.returncode
        else:
            out, err = p.communicate()
            return out.decode("utf-8"), err.decode("utf-8"), p.returncode
    except subprocess.TimeoutExpired:
        print("Process automatically teminated after " + str(timeOut) + " sec")
        p.terminate()
        return "** Timeout: " + str(timeOut) + " sec **", "", 0
    except Exception as e:
        print("An error occurred:" + str(e))
        return "", "", -1


def icmd(cmd_seq, sh=False):
    try:
        p = subprocess.Popen(
            cmd_seq, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE, shell=sh
        )
        return p
    except Exception as e:
        print("An error occurred:" + str(e))
        return "", "", -1


def xcmd(cmd_seq, sh=False):
        process = icmd(cmd_seq, sh)

        while True:
            output = clearText(process.stdout.readline().decode("utf-8"))

            if output == '' and process.poll() is not None:
                break
            if output:
                print(output)