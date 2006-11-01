from itertools import izip, count

Inf = 1e100

def search(board, depth):
    return alphabeta(board, depth, -Inf, Inf)

def alphabeta(board, depth, alpha, beta):
    if depth == 0: return board.evaluate()

    if board.dwarfturn:
        for play in board.dwarfplays():
            undo = board.do(play)
            beta = min(beta, alphabeta(board, depth-1, alpha, beta))
            undo()
            if alpha >= beta: return alpha

        return beta
    else:
        for play in board.trollplays():
            undo = board.do(play)
            alpha = max(alpha, alphabeta(board, depth-1, alpha, beta))
            undo()
            if alpha >= beta: return beta

        return alpha

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

    >>> b = ThudBoard("d.. "
    ...               "... "
    ...               "... ")
    >>> sorted(b.dwarfmovesfrom(0,0))
    [(0, 1), (0, 2), (1, 0), (1, 1), (2, 0), (2, 2)]

    >>> b = ThudBoard("d.. "
    ...               ".#. "
    ...               "... ")
    >>> sorted(b.dwarfmovesfrom(0,0))
    [(0, 1), (0, 2), (1, 0), (2, 0)]

    >>> b = ThudBoard("dT. "
    ...               "... "
    ...               "..T ")
    >>> sorted(b.dwarfattacksfrom(0,0))
    [(1, 0)]

    >>> b = ThudBoard("d.d.. "
    ...               ".dd.. "
    ...               "ddddT "
    ...               "...#. "
    ...               "..T.T ")
    >>> sorted(b.dwarfattacksfrom(2,2))
    [(2, 4)]

    >>> b = ThudBoard("ddd... "
    ...               "ddd.TT ")
    >>> sorted(b.dwarfattacksfrom(2,1))
    [(4, 1)]

    >>> b = ThudBoard("T.. "
    ...               "... "
    ...               "... ")
    >>> sorted(b.trollmovesfrom(0,0))
    [(0, 1), (1, 0), (1, 1)]

    >>> b = ThudBoard("T.. "
    ...               ".#. "
    ...               "... ")
    >>> sorted(b.trollmovesfrom(0,0))
    [(0, 1), (1, 0)]

    >>> b = ThudBoard("T.d "
    ...               "... "
    ...               "... "
    ...               "..d ")
    >>> sorted(b.trollattacksfrom(0,0))
    [(1, 0), (1, 1)]

    >>> b = ThudBoard("TT... "
    ...               "TT.Td "
    ...               "..... "
    ...               "...#. "
    ...               ".d..d ")
    >>> sorted(b.trollattacksfrom(1,1))
    [(1, 3)]

    >>> b = ThudBoard("T")
    >>> b.evaluate()
    4000

    >>> b = ThudBoard("dd.d")
    >>> b.evaluate()
    -3002
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

    def dwarfmovesfrom(self, x, y):
        occupied = self.dwarfat | self.trollat | self.wallat

        xsize,ysize = self.size
        for xdir,ydir in ((dx,dy) for dx in [-1,0,1] for dy in [-1,0,1]):
            if (xdir,ydir) == (0,0): continue
            for dist in count(1):
                tx,ty = x + xdir*dist, y + ydir*dist
                if (0 <= tx < xsize and 0 <= ty < ysize
                    and not occupied[tx,ty]):
                    yield tx,ty
                else: break

    def dwarfattacksfrom(self, x, y):
        occupied = self.dwarfat | self.trollat | self.wallat

        xsize,ysize = self.size
        for xdir,ydir in ((dx,dy) for dx in [-1,0,1] for dy in [-1,0,1]):
            if (xdir,ydir) == (0,0): continue
            for dist in count(1):
                dx,dy = xdir*dist, ydir*dist
                tx,ty = x + dx, y + dy
                if 0 <= tx < xsize and 0 <= ty < ysize:
                    if (self.dwarfat[x - dx + xdir, y - dy + ydir]
                        and self.trollat[tx,ty]): yield tx,ty
                    if occupied[tx,ty]: break
                else: break

    def trollmovesfrom(self, x, y):
        occupied = self.dwarfat | self.trollat | self.wallat

        xsize,ysize = self.size
        for xdir,ydir in ((dx,dy) for dx in [-1,0,1] for dy in [-1,0,1]):
            if (xdir,ydir) == (0,0): continue
            tx,ty = x + xdir, y + ydir
            if (0 <= tx < xsize and 0 <= ty < ysize
                and not occupied[tx,ty]):
                yield tx,ty

    def trollattacksfrom(self, x, y):
        occupied = self.dwarfat | self.trollat | self.wallat
        dwarfnear = self.dwarfat.neighbors()

        xsize,ysize = self.size
        for xdir,ydir in ((dx,dy) for dx in [-1,0,1] for dy in [-1,0,1]):
            if (xdir,ydir) == (0,0): continue
            for dist in count(1):
                dx,dy = xdir*dist, ydir*dist
                tx,ty = x + dx, y + dy
                if (0 <= tx < xsize and 0 <= ty < ysize
                    and self.trollat[x - dx + xdir, y - dy + ydir]
                    and not occupied[tx,ty]):
                    if dwarfnear[tx,ty]: yield tx,ty
                else: break

    def evaluate(self):
        material = (4000 * self.trollat.census()
                    - 1000 * self.dwarfat.census())

        xsize,ysize = self.size
        temp = BitBoard(xsize, [False] * (xsize * ysize))
        clump = 0
        for pos in self.dwarfat.positions():
            temp[pos] = True
            clump += (temp.neighbors() & self.dwarfat).census()
            temp[pos] = False

        return material - clump

    def dwarfplays(self):
        pass

    def trollplays(self):
        pass

    def do(self, play):
        pass

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

def _test():
    import doctest
    failed,_ = doctest.testmod()
    if not failed:
        print "OK"

if __name__ == "__main__":
    _test()
