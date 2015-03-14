Gap buffers are efficient mutable sequences. They are most often used to store text in text editors. They utilize locality of modification to avoid copying large amounts of data and allocate extra elements to avoid memory allocation dominating performance. The extra allocated items provide a movable gap between the two parts that contain data. Insertions and deletions occur at the gap.

For a description of gap buffers see [Data Structures in a Bit-Mapped Text Editor](http://www.cs.cmu.edu/~wjh/papers/byte.html), Wilfred J. Hanson, Byte January 1987

The item type for the gap buffer may be character, Unicode character, or integer and is determined from the type of the constructor argument with a list interpreted as integer:
```
>>> from gapbuffer import GapBuffer
>>> print GapBuffer('The life of Brian')
The life of Brian
>>> print GapBuffer(u'Mr Creosote')
Mr Creosote
>>> print GapBuffer([1,2,3])
GapBuffer('i') [1, 2, 3]
```

The example use Python 2.x syntax although GapBuffer will also work with Python 3.x.

GapBuffer implements Python's sequence protocol:
```
>>> movie = GapBuffer('The life of Brian')
>>> movie[:] = 'The meaning - with Life'; print movie
The meaning - with Life
>>> del movie[12:14]; print movie
The meaning with Life
>>> movie[4] = 'M'; print movie
The Meaning with Life
>>> movie[12:16] = 'of'; print movie
The Meaning of Life
>>> print movie[0:3]
The
>>> print len(movie)
19
```

GapBuffer has insert and extend methods similar to Python lists:
```
>>> movie.insert(0, '\'')
>>> movie.extend('\'!')
>>> print movie
'The Meaning of Life'!
```

Portions can be retrieved as strings directly rather than by a slice and conversion to avoid copying twice with retrieve(start, length):
```
>>> print movie.retrieve(5,7)
Meaning
```

A GapBuffer will not release memory unless asked:
```
>>> print movie.size
25
>>> movie[:] = 'ab'; print movie.size
25
>>> movie.slim(); print movie.size
8
```

The values of a segment may be added to with increment(start, length, value). This is useful for maintaining the starting position of every line in a document, for example.
```
>>> positions = GapBuffer([100, 140, 220, 280])
>>> positions.increment(1,3,-7)
>>> print positions
GapBuffer('i') [100, 133, 213, 273]
```

The buffer protocol is implemented which allows use with features such as regular expression searches and writing to file:
```
>>> import re
>>> movie = GapBuffer(u'The life of Brian')
>>> print movie
The life of Brian
>>> r = re.compile('B[a-z]+', re.M)
>>> where = r.search(movie)
>>> print where.group(0)
Brian
```

# Issues #

More item types could be implemented, possibly all of those available from the array module with a typecode keyword parameter to the constructor specifying the item type. Possibly use different names rather than a typecode: CharDoc, UnicodeDoc.

The code is too messy with detailed knowledge of the data structure spread throughout the code. This should be regularized as should be the consumption of arguments so it is easier to add methods and item types.

# Code #

[Unit tests for gapbuffer](http://www.scintilla.org/unitTests.py)

[Demonstration code](http://www.scintilla.org/gbdemonstration.py)


# Discussion #

Since this is a small simple module there will not be a mailing list. The news group comp.lang.python is probably the best place to talk about gapbuffer. Do not send messages about gapbuffer to my personal email address.