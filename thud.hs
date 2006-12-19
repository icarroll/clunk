{-
board
    undirected graph of board locations
        labeled directions
    initial side on move
    initial placement of pieces on each side

position
    side on move
    set of occupied nodes on each side
    calculated data
        hash
        dwarf blocks
        capture threats
        shoving/throwing lines
-}

import Data.Set
import Data.Graph.Inductive

data Side = Dwarf | Troll
    deriving (Show,Eq,Ord,Enum,Bounded)
sides :: Set Side
sides = fromList [minBound ..]

data Direction = North | Northeast | East | Southeast
               | South | Southwest | West | Northwest
    deriving (Show,Eq,Ord,Enum,Bounded)
directions :: Set Direction
directions = fromList [minBound ..]

data Column = A | B | C | D | E | F | G | H | J | K | L | M | N | O | P
    deriving (Show,Eq,Ord,Enum,Bounded)
type Row = Int
type Location = (Column,Row)

data Board = Board (Gr Location Direction) Side [Set Location]

data Position = Position Side [Set Location]

type Distance = Int
data Capture = NoCapt
             | DestCapt
             | NeighborCapt (Set Direction)
type Move = (Side,Location,Direction,Distance,Capture)

