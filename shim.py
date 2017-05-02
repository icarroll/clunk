#!/usr/bin/env python3

from subprocess import Popen, PIPE
import sys

COMMAND = ["python3", "console.py", "next_move"]

BOARD_SETUP = b"dF1,dG1,dJ1,dK1,dE2,dL2,dD3,dM3,dC4,dN4,dB5,dO5,dA6,dP6,dA7,dP7,dA9,dP9,dA10,dP10,dB11,dO11,dC12,dN12,dD13,dM13,dE14,dL14,dF15,dG15,dJ15,dK15,TG7,TH7,TJ7,TG8,TJ8,TG9,TH9,TJ9,RH8\n"

side = input()

moves = []
while True:
    try:
        subproc = Popen(COMMAND, stdin=PIPE, stdout=PIPE)
        subproc.stdin.write(BOARD_SETUP)
        for move in moves:
            subproc.stdin.write(move)
            subproc.stdin.write(b"\n")
        subproc.stdin.close()
        mymove = subproc.stdout.read().strip()
    finally:
        subproc.stdin.close()
        subproc.stdout.close()
        subproc.terminate()
        try:
            subproc.wait(timeout=1)
        except TimeoutExpired:
            subproc.kill()

    moves.append(mymove)
    sys.stdout.write(str(mymove, "utf-8"))
    sys.stdout.write("\n")
    sys.stdout.flush()

    yourmove = sys.stdin.readline().strip().replace(" ", "").encode("utf-8")
    moves.append(yourmove)
