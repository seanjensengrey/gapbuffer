# Demonstration of GapBuffer type
from gapbuffer import GapBuffer
print GapBuffer("The life of Brian")
print GapBuffer(u"Mr Creosote")
print GapBuffer([1,2,3])

# Sequence protocol
movie = GapBuffer("The life of Brian")
movie[:] = "The meaning - with Life"; print movie
del movie[12:14]; print movie
movie[4] = "M"; print movie
movie[12:16] = "of"; print movie
print movie[0:3]
print len(movie)

# GapBuffer Methods
movie.insert(0, "\'")
movie.extend("\'!")
print movie
print movie.retrieve(5,7)
print movie.size
movie[:] = "ab"; print movie.size
movie.slim(); print movie.size

positions = GapBuffer([100, 140, 220, 280])
positions.increment(1,3,-7)
print positions

# Buffer protocol
import re
movie = GapBuffer(u"The life of Brian")
print movie
r = re.compile("B[a-z]+", re.M)
where = r.search(movie)
print where.group(0)
