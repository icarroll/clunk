import sys
from subprocess import Popen,PIPE
from itertools import izip

def main(dwarfcmd=None, trollcmd=None):
    board = ThudBoard()

    if dwarfcmd is None:
        dwarfcmd = raw_input("Dwarf player: ")
    dwarf = Popen(dwarfcmd.split(), stdin=PIPE, stdout=PIPE)
    print >>dwarf.stdin, "d"

    if trollcmd is None:
        trollcmd = raw_input("Troll player: ")
    troll = Popen(trollcmd.split(), stdin=PIPE, stdout=PIPE)
    print >>troll.stdin, "T"

    while True:
        move = dwarf.stdout.readline()
        troll.stdin.write(move)
        sys.stdout.write(move)

        move = troll.stdout.readline()
        dwarf.stdin.write(move)
        sys.stdout.write(move)

class ThudBoard:
    """
    >>> b = ThudBoard()
    >>> print b.show()
       A B C D E F G H J K L M N O P
     1 # # # # # d d . d d # # # # #
     2 # # # # d . . . . . d # # # #
     3 # # # d . . . . . . . d # # #
     4 # # d . . . . . . . . . d # #
     5 # d . . . . . . . . . . . d #
     6 d . . . . . . . . . . . . . d
     7 d . . . . . T T T . . . . . d
     8 . . . . . . T # T . . . . . .
     9 d . . . . . T T T . . . . . d
    10 d . . . . . . . . . . . . . d
    11 # d . . . . . . . . . . . d #
    12 # # d . . . . . . . . . d # #
    13 # # # d . . . . . . . d # # #
    14 # # # # d . . . . . d # # # #
    15 # # # # # d d . d d # # # # #
    """
    COLUMNS = "".join(chr(e) for e in range(ord("A"), ord("Z")+1)
                      if e != ord("I"))

    def __init__(self, start=None):
        if start is None:
            start = """#####dd.dd#####
                       ####d.....d####
                       ###d.......d###
                       ##d.........d##
                       #d...........d#
                       d.............d
                       d.....TTT.....d
                       ......T#T......
                       d.....TTT.....d
                       d.............d
                       #d...........d#
                       ##d.........d##
                       ###d.......d###
                       ####d.....d####
                       #####dd.dd#####"""
        start = start.split()

        xsize = len(start[0])
        ysize = len(start)
        self.size = (xsize,ysize)

        self.dwarfbits = []
        self.trollbits = []
        self.wallbits = []
        for row in start:
            for c in row:
                self.dwarfbits += [c == 'd']
                self.trollbits += [c == 'T']
                self.wallbits += [c == '#']

        self.dwarfat = BitBoard(xsize, self.dwarfbits)
        self.trollat = BitBoard(xsize, self.trollbits)
        self.wallat = BitBoard(xsize, self.wallbits)

        self.dwarfturn = True

        self.DWARF = "d"
        self.TROLL = "T"

        self.WHICH = {self.DWARF:(self.dwarfat, self.trollat),
                      self.TROLL:(self.trollat, self.dwarfat)}

    def show(self):
        spaces = izip(self.dwarfbits, self.trollbits, self.wallbits)
        char = {(True,False,False):"d",
                (False,True,False):"T",
                (False,False,True):"#",
                (False,False,False):".",}

        xsize,ysize = self.size
        cols = self.COLUMNS[:xsize]

        lines = []
        lines += ["   " + " ".join(cols)]
        for row in range(1, ysize+1):
            line = str(row).rjust(2)
            for col in cols: line += " " + char[spaces.next()]
            lines += [line]

        return "\n".join(lines)

    def do(self, move):
        we,they = self.WHICH[move.side]

        we[move.frompos] = False
        for capt in move.capts:
            they[capt] = False
        we[move.topos] = True

        self.dwarfturn = move.side == self.TROLL

    def undo(self, move):
        we,they = self.WHICH[move.side]

        we[move.topos] = False
        for capt in move.capts:
            they[capt] = True
        we[move.frompos] = True

        self.dwarfturn = move.side == self.DWARF

class BitBoard:
    """
    """
    def __init__(self, xsize, data):
        self.xsize = xsize
        self.ysize = len(data) / xsize
        self.data = data

    def __getitem__(self, (x,y)):
        return self.data[self.xsize * y + x]

    def __setitem__(self, (x,y), value):
        self.data[self.xsize * y + x] = value

    def __and__(self, other):
        return BitBoard(self.xsize,
                        [s and o for s,o in zip(self.data, other.data)])

    def __or__(self, other):
        return BitBoard(self.xsize,
                        [s or o for s,o in zip(self.data, other.data)])

    def census(self):
        return sum(self.data)

    def positions(self):
        posns = (divmod(n, self.xsize) for n,e in enumerate(self.data) if e)
        return ((x,y) for y,x in posns)

    def neighbors(self):
        newboard = BitBoard(self.xsize, [False] * len(self.data))

        for x,y in self.positions():
            for tx,ty in ((x+dx,y+dy) for dx in [-1,0,1] for dy in [-1,0,1]):
                if (tx,ty) == (x,y): continue
                if 0 <= tx < self.xsize and 0 <= ty < self.ysize:
                    newboard[tx,ty] = True

        return newboard

    def neighborsof(self, x, y):
        newboard = BitBoard(self.xsize, [False] * len(self.data))

        for tx,ty in ((x+dx,y+dy) for dx in [-1,0,1] for dy in [-1,0,1]):
            if (tx,ty) == (x,y): continue
            if 0 <= tx < self.xsize and 0 <= ty < self.ysize:
                newboard[tx,ty] = True

        return newboard

class Move:
    def __init__(self, side, frompos, topos, capts):
        self.side = side
        self.frompos = frompos
        self.topos = topos
        self.capts = capts

    def __str__(self):
        temp = [self.side, strpos(self.frompos), strpos(self.topos)]
        for capt in self.capts:
            temp += ["x " + strpos(capt)]
        return " ".join(temp)

def strpos((x,y)):
    return Board.COLUMNS[x] + str(y+1)

def _test():
    import doctest
    failed,_ = doctest.testmod()
    if not failed:
        print "OK"

if __name__ == "__main__":
    if "-t" in sys.argv:
        _test()
    else:
        main(*sys.argv[1:])
