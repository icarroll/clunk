{-
calculated data
    hash
    dwarf blocks
    capture threats
    shoving/throwing lines
-}

import qualified Data.Set as Set
import Data.Set (Set)
import qualified Data.Map as Map
import Data.Map (Map)
import Data.Graph.Inductive

data Setup = Setup Side Board Pieces
standard = Setup Dwarf b p
    where
        b = 
        p = 
        layout = ["#####dd.dd#####",
                  "####d.....d####",
                  "###d.......d###",
                  "##d.........d##",
                  "#d...........d#",
                  "d.............d",
                  "d.....TTT.....d",
                  "......T#T......",
                  "d.....TTT.....d",
                  "d.............d",
                  "#d...........d#",
                  "##d.........d##",
                  "###d.......d###",
                  "####d.....d####",
                  "#####dd.dd#####"]

data Position = Position {onmove :: Side, pieces :: Pieces}

data Move = Move Side Location Direction Distance Capture

type Distance = Int
data Capture = NoCapt
             | DestCapt
             | NeighborCapt (Set Direction)

type Board = Gr Location Direction

data Side = Dwarf | Troll
    deriving (Show,Eq,Ord,Enum,Bounded)
sides :: Set Side
sides = Set.fromList [minBound ..]
frob 'd' = Dwarf
frob 'T' = Troll
defrob Dwarf = 'd'
defrob Troll = 'T'

type Pieces = Map Side (Set Location)

data Location = Location {col :: Int, row :: Int}
instance Show Location
    where show (Location col row) = (columns !! col) : show (row + 1)

columns = filter (/= 'I') ['A' ..]

data Direction = North | Northeast | East | Southeast
               | South | Southwest | West | Northwest
    deriving (Show,Eq,Ord,Enum,Bounded)
directions :: Set Direction
directions = Set.fromList [minBound ..]
