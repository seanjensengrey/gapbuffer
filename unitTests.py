# -*- coding: utf-8 -*-

# A set of basic unit tests for gap buffers of all three type, string, unicode and integer.
# Requires Python 2.6 or newer as it uses byte literals

import re, sys, unittest

# Define a function to convert a quoted literal string, which is a byte string on 2.x and
# and a Unicode string on 3.x into a Unicode string
if sys.version_info[0] >= 3:
	def u(t):
		return t
else:
	def u(t):
		return unicode(t, "utf-8")

# Retrieve the string data from a GapBuffer
def r(gb):
	return gb.retrieve(0, len(gb))

from gapbuffer import GapBuffer

class TestString(unittest.TestCase):

	def setUp(self):
		self.x = GapBuffer(b"")
		self.testVal = b"abc"

	def testInitWithValue(self):
		o = GapBuffer(self.testVal)
		self.assertEquals(r(o), self.testVal)

	def testLength(self):
		self.assertEquals(len(self.x), 0)

	def testAssign(self):
		self.x[:] = self.testVal
		self.assertEquals(len(self.x), 3)

	def testCompare(self):
		self.x[:] = self.testVal
		o = GapBuffer(b"")
		o[:] = self.testVal
		self.assertEquals(self.x, o)
		self.assert_(self.x <= o)
		self.assert_(o >= o)

		o[:] = b"bbc"
		self.assert_(self.x != o)
		self.assert_(self.x < o)
		self.assert_(self.x <= o)
		self.assert_(o > self.x)
		self.assert_(o >= o)

		o[:] = b"abcd"
		self.assert_(self.x != o)
		self.assert_(self.x < o)
		self.assert_(self.x <= o)
		self.assert_(o > self.x)
		self.assert_(o >= o)

		self.assert_(o != None)

	def testRetrieve(self):
		self.x[:] = self.testVal
		self.assertEquals(self.x.retrieve(0,3), self.testVal)

	def testConvert(self):
		self.x[:] = self.testVal
		if sys.version_info[0] >= 3:
			# Since byte strings are not the native type on 3.x a representation inside b'' is returned
			# Just like str(b'abc') returns "b'abc'"
			self.assertEquals(str(self.x), "b'abc'")
		else:
			self.assertEquals(str(self.x), self.testVal)

	def testExtend(self):
		self.x[:] = self.testVal
		self.x.extend(b"d")
		self.assertEquals(r(self.x), b"abcd")

	def testInsert(self):
		self.x[:] = self.testVal
		self.x.insert(1, b"!@")
		self.assertEquals(r(self.x), b"a!@bc")

	def testIncrement(self):
		self.x[:] = self.testVal
		self.x.increment(1,1,1)
		self.assertEquals(r(self.x), b"acc")

	def testDel(self):
		self.x[:] = self.testVal
		del self.x[1:2]
		self.assertEquals(r(self.x), b"ac")
		del self.x[1]
		self.assertEquals(r(self.x), b"a")

	def testDelAll(self):
		self.x[:] = self.testVal
		del self.x[:]
		self.assertEquals(r(self.x), b"")

	def testReplace(self):
		self.x[:] = b"abcde"
		self.x[2:4] = b"!"
		self.assertEquals(r(self.x), b"ab!e")

	def testAt(self):
		self.x[:] = self.testVal
		self.assertEquals(self.x[2], b"c")

	def testSetAt(self):
		self.x[:] = self.testVal
		self.x[1] = b"B"
		self.assertEquals(r(self.x), b"aBc")

	def testSlice(self):
		self.x[:] = b"abcd"
		sl = self.x[1:3]
		a = GapBuffer(b"")
		a[:] = b"bc"
		self.assertEquals(sl, a)

	def testConcat(self):
		self.x[:] = self.testVal
		co = self.x + self.x
		self.assertEquals(r(co), b"abcabc")

	def testRepeat(self):
		self.x[:] = self.testVal
		co = self.x * 3
		self.assertEquals(r(co), b"abcabcabc")

	def testRE(self):
		self.x[:] = self.testVal
		rg = re.compile(b"b[a-e]*", re.M)
		y = rg.search(self.x)
		self.assertEquals(r(y.group(0)), b"bc")

	def testSlim(self):
		self.x[:] = self.testVal
		self.x.slim()
		self.assertEquals(r(self.x), self.testVal)

class TestStringExceptions(unittest.TestCase):

	def setUp(self):
		self.x = GapBuffer(b"")
		self.testVal = b"abc"

	def testRetrieve(self):
		self.x[:] = self.testVal
		self.assertRaises(IndexError, self.x.retrieve, 0, 4)

	def testExtend(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.extend, 0)

	def testInsert(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.insert, 0, 0)
		self.assertRaises(IndexError, self.x.insert, 100, b"a")

	def testIncrement(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.increment, 0, 1, b'a')
		self.assertRaises(IndexError, self.x.increment, 1, 100, 1)

class TestUnicode(unittest.TestCase):

	def setUp(self):
		self.x = GapBuffer(u(""))
		self.testVal = u("abc")

	def testInitWithValue(self):
		o = GapBuffer(u("a\x0123"))
		self.assertEquals(str(o), u("a\x0123"))

	def testLength(self):
		self.assertEquals(len(self.x), 0)

	def testAssign(self):
		self.x[:] = self.testVal
		self.assertEquals(len(self.x), 3)

	def testCompare(self):
		self.x[:] = self.testVal
		o = GapBuffer(u(""))
		o[:] = self.testVal
		self.assertEquals(self.x, o)
		self.assert_(self.x <= o)
		self.assert_(o >= o)

		o[:] = u("bbc")
		self.assert_(self.x != o)
		self.assert_(self.x < o)
		self.assert_(self.x <= o)
		self.assert_(o > self.x)
		self.assert_(o >= o)

		o[:] = u("abcd")
		self.assert_(self.x != o)
		self.assert_(self.x < o)
		self.assert_(self.x <= o)
		self.assert_(o > self.x)
		self.assert_(o >= o)

		self.assert_(o != None)

	def testRetrieve(self):
		self.x[:] = self.testVal
		self.assertEquals(self.x.retrieve(0,3), self.testVal)

	def testConvert(self):
		self.x[:] = self.testVal
		self.assertEquals(str(self.x), self.testVal)

	def testExtend(self):
		self.x[:] = self.testVal
		self.x.extend(u("d"))
		self.assertEquals(str(self.x), u("abcd"))

	def testInsert(self):
		self.x[:] = self.testVal
		self.x.insert(1, u("!@"))
		self.assertEquals(str(self.x), u("a!@bc"))

	def testIncrement(self):
		self.x[:] = self.testVal
		self.x.increment(1,1,1)
		self.assertEquals(str(self.x), u("acc"))

	def testDel(self):
		self.x[:] = self.testVal
		del self.x[1:2]
		self.assertEquals(str(self.x), u("ac"))
		del self.x[1]
		self.assertEquals(str(self.x), u("a"))

	def testDelAll(self):
		self.x[:] = self.testVal
		del self.x[:]
		self.assertEquals(str(self.x), u(""))

	def testReplace(self):
		self.x[:] = u("abcde")
		self.x[2:4] = u("!")
		self.assertEquals(str(self.x), u("ab!e"))

	def testAt(self):
		self.x[:] = self.testVal
		self.assertEquals(self.x[2], u("c"))

	def testSetAt(self):
		self.x[:] = self.testVal
		self.x[1] = u("B")
		self.assertEquals(str(self.x), u("aBc"))

	def testSlice(self):
		self.x[:] = u("abcd")
		sl = self.x[1:3]
		a = GapBuffer(u(""))
		a[:] = u("bc")
		self.assertEquals(sl, a)

	def testConcat(self):
		self.x[:] = self.testVal
		co = self.x + self.x
		self.assertEquals(str(co), u("abcabc"))

	def testRepeat(self):
		self.x[:] = self.testVal
		co = self.x * 3
		self.assertEquals(str(co), u("abcabcabc"))

	def testRE(self):
		self.x[:] = self.testVal
		r = re.compile("b[a-e]*", re.M)
		y = r.search(self.x)
		self.assertEquals(str(y.group(0)), u("bc"))

	def testRussianRE(self):
		self.x[:] = u("Палить из пушки по воробьям")
		rusWord = u("пушки")
		rg = re.compile("\w+", re.M|re.U)
		y = rg.findall(self.x)
		self.assertEquals(r(y[2]), rusWord)

	def testSlim(self):
		self.x[:] = self.testVal
		self.x.slim()
		self.assertEquals(str(self.x), self.testVal)

class TestUnicodeExceptions(unittest.TestCase):

	def setUp(self):
		self.x = GapBuffer(u(""))
		self.testVal = u("abc")

	def testRetrieve(self):
		self.x[:] = self.testVal
		self.assertRaises(IndexError, self.x.retrieve, 0, 4)

	def testExtend(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.extend, 0)

	def testInsert(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.insert, 0, 0)
		self.assertRaises(IndexError, self.x.insert, 100, u("a"))

	def testIncrement(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.increment, 0, 1, u("a"))
		self.assertRaises(IndexError, self.x.increment, 1, 100, 1)

class TestInteger(unittest.TestCase):

	def setUp(self):
		self.x = GapBuffer([])
		self.testVal = [1, 2, 3]

	def testInitWithValue(self):
		o = GapBuffer(self.testVal)
		self.assertEquals(list(o), self.testVal)

	def testLength(self):
		self.assertEquals(len(self.x), 0)

	def testAssign(self):
		self.x[:] = [1, 2, 3]
		self.assertEquals(len(self.x), 3)

	def testCompare(self):
		self.x[:] = self.testVal
		o = GapBuffer([])
		o[:] = self.testVal
		self.assertEquals(self.x, o)
		self.assert_(self.x <= o)
		self.assert_(o >= o)

		o[:] = [2, 2, 3]
		self.assert_(self.x != o)
		self.assert_(self.x < o)
		self.assert_(self.x <= o)
		self.assert_(o > self.x)
		self.assert_(o >= o)

		o[:] = [1, 2, 3, 4]
		self.assert_(self.x != o)
		self.assert_(self.x < o)
		self.assert_(self.x <= o)
		self.assert_(o > self.x)
		self.assert_(o >= o)

		self.assert_(o != None)

	def testConvert(self):
		self.x[:] = self.testVal
		self.assertEquals(str(self.x), "GapBuffer('i') [1, 2, 3]")

	def testExtend(self):
		self.x[:] = self.testVal
		self.x.extend([4])
		self.assertEquals(list(self.x), [1, 2, 3, 4])

	def testInsert(self):
		self.x[:] = self.testVal
		self.x.insert(1, [22,33])
		self.assertEquals(list(self.x), [1, 22, 33, 2, 3])

	def testIncrement(self):
		self.x[:] = self.testVal
		self.x.increment(1,1,1)
		self.assertEquals(list(self.x), [1, 3, 3])

	def testDel(self):
		self.x[:] = self.testVal
		del self.x[1:2]
		self.assertEquals(list(self.x), [1, 3])
		del self.x[1]
		self.assertEquals(list(self.x), [1])

	def testDelAll(self):
		self.x[:] = self.testVal
		del self.x[:]
		self.assertEquals(list(self.x), [])

	def testReplace(self):
		self.x[:] = [1, 2, 3, 4, 5]
		self.x[2:4] = [55]
		self.assertEquals(list(self.x), [1, 2, 55, 5])

	def testAt(self):
		self.x[:] = self.testVal
		self.assertEquals(self.x[2], 3)

	def testSetAt(self):
		self.x[:] = self.testVal
		self.x[1] = 22
		self.assertEquals(list(self.x), [1, 22, 3])

	def testSlice(self):
		self.x[:] = [1, 2, 3, 4]
		sl = self.x[1:3]
		a = GapBuffer([])
		a[:] = [2, 3]
		self.assertEquals(sl, a)

	def testSliceAll(self):
		self.x[:] = [1, 2, 3, 4]
		sl = self.x[:]
		a = GapBuffer([])
		a[:] = [1, 2, 3, 4]
		self.assertEquals(sl, a)

	def testConcat(self):
		self.x[:] = self.testVal
		co = self.x + self.x
		self.assertEquals(list(co), [1, 2, 3, 1, 2, 3])

	def testRepeat(self):
		self.x[:] = self.testVal
		co = self.x * 3
		self.assertEquals(list(co), [1, 2, 3, 1, 2, 3, 1, 2, 3])

	def testSlim(self):
		self.x[:] = self.testVal
		self.x.slim()
		self.assertEquals(list(self.x), self.testVal)


class TestIntegerExceptions(unittest.TestCase):

	def setUp(self):
		self.x = GapBuffer([])
		self.testVal = [1, 2, 3]

	def testRetrieve(self):
		self.x[:] = self.testVal
		self.assertRaises(IndexError, self.x.retrieve, 0, 4)

	def testExtend(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.extend, 0)

	def testInsert(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.insert, 0, 0)
		self.assertRaises(IndexError, self.x.insert, 100, [1])

	def testIncrement(self):
		self.x[:] = self.testVal
		self.assertRaises(TypeError, self.x.increment, 0, 1, "a")
		self.assertRaises(IndexError, self.x.increment, 1, 100, 1)

if __name__ == '__main__':
	unittest.main()