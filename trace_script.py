#!/usr/bin/env python3
import sys
import subprocess

class Tracer:

    traceDirectory = "traces"
    main = "./main"
    command = main
    useValgrind = False
    colored = False

    traceDict = {
        1: "trace-1",
        2: "trace-2"
    }

    traceProbs = {
        1: "Trace-01",
        2: "Tracce-01"
    }

    RED = '\033[91m'
    GREEN = '\033[92m'
    WHITE = '\033[0m'

    def __init__(self,
                 useValgrind=False,
                 colored=False):
        self.useValgrind = useValgrind
        self.colored = colored
    
    def printInColor(self, text, color):
        if self.colored == False:
            color = self.WHITE
        print(color, text, self.WHITE, sep = '')

    def runTrace(self, tid):
        if not tid in self.traceDict:
            self.printInColor("ERROR: No trace with id %d" % tid, self.RED)
            return False
        fname = "%s/%s.cmd" % (self.traceDirectory, self.traceDict[tid])
        clist = self.command

        try:
            with open(fname, 'r') as infile:
                retcode = subprocess.call(clist, stdin=infile)
        except Exception as e:
            self.printInColor("Call of '%s' failed: %s" % (" ".join(clist), e), self.RED)
            return False
        return retcode == 0

    def run(self, tid=0):
        print("---\tTrace\t\t")
        if tid == 0:
            tidList = self.traceDict.keys()
        else:
            if not tid in self.traceDict:
                self.printInColor("ERROR: Invalid trace ID %d" % tid, self.RED)
                return
            tidList = [tid]
        if self.useValgrind:
            self.command = ['valgrind']
        for t in tidList:
            tname = self.traceDict[t]
            print("+++ TESTING %s:" % tname)
            ok = self.runTrace(t)

def run(name, args):
    prog = ""
    useValgrind = False
    colored = False
    tid = 0

    tracer = Tracer(useValgrind=useValgrind, colored=colored)
    tracer.run(tid)


if __name__ == "__main__":
    run(sys.argv[0], sys.argv[1:])
