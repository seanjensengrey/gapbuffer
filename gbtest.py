import array
import re
import gapbuffer
print dir(gapbuffer)
x = gapbuffer.GapBuffer(u"")
#~ print dir(x)
#~ print x
#~ print x.size
print x
x.insert(0,u"ree trunk drinking tube")
#~ print ":", x.text()
#~ print x.retrieve(3,5)
del x[1:3]
#~ print ":", x.text()
f = open("x.txt", "wb")
f.write(x)
f.close()

r = re.compile("t.", re.M)

#~ y = r.search("\nate", 1)
#~ print y, y.group(0)

#~ zz = array.array("c", "ao te aroa")
#~ print zz
#~ y = r.search(zz)
#~ print y, y.start(0), y.end(0)

#~ zz = array.array("u", u"ao te aroa")
#~ print zz
#~ y = r.search(zz)
#~ print y, y.start(0), y.end(0)

#~ y = r.search(x)
#~ print y, y.start(0), y.end(0)
#~ print y.group(0)

#~ print x

#~ tr = [(t.start(0), t.end(0)) for t in re.finditer(r, x)]
#~ print tr
#~ diff = 0
#~ for (s,e) in tr:
	#~ x.insert(s+diff, "!")
	#~ diff += 1
	#~ print s, e

#~ print x, dir(x)
#~ print x.size
#~ print len(x)
#~ print x.gapLength, x.part1Length, x.growSize, x.itemsize
#~ print x.bufferAppearence

#~ x.bufferAppearence = 1
#~ print x.bufferAppearence
tr = [(t.start(0), t.end(0)) for t in re.finditer(r, x)]
print tr


#~ print x[1], x[2]

#~ print x[2:7].text()

#~ xee = gapbuffer.GapBuffer(u"")
#~ print xee.itemsize, len(xee)
#~ xee.insert(0, u"lots of tests")
#~ y = r.search(xee)
#~ print y, y.start(0), y.end(0)

#~ del xee[5:8]
#~ print xee.itemsize, len(xee)

#~ u = xee.retrieve(2,6)
#~ print repr(u)
#~ print xee
#~ print xee[2]
#~ print xee[2:6]

xee = gapbuffer.GapBuffer(u"")
print xee.itemsize, len(xee)
xee.insert(0, u"lots of tests")
y = r.search(xee)
print y, y.start(0), y.end(0)

del xee[5:8]
print xee.itemsize, len(xee)

u = xee.retrieve(2,6)
print repr(u)
print xee
print xee[2]
print xee[2:6]

xee[1:3] = u"***"
print xee

zed = xee[1:4]
print zed, type(zed)

xee[1:4] = u""
print xee

xee[1:1] = zed
print xee

xee[2] = u'!'
print xee

#~ xee = xee + xee

import cStringIO
import time

tv = 'A first line.\n'
iters = 1000000

start = time.time()
output = cStringIO.StringIO()
for i in xrange(iters):
	output.write(tv)
end = time.time()
contents = output.getvalue()

output.close()

ooo = gapbuffer.GapBuffer("")

#~ tv = gapbuffer.GapBuffer("")
#~ tv[:] = 'A longer than expected first line.\n'
#~ tv = 'A first line.\n'

ilen = len(tv)
endpos = 0
startt = time.time()
for i in xrange(iters):
	#~ ooo.extend(tv)
	#~ ooo.insert(endpos, tv)
	ooo[endpos:endpos] = tv
	endpos += ilen
endt = time.time()
ct = str(ooo)

print "StringIO", (end - start)
print "GapBuffer", (endt - startt)


print (ct == contents)

print len(ooo), ooo.size, ooo.growSize
del ooo[1:len(ooo) - 1]
ooo.slim()
print len(ooo), ooo.size, ooo.growSize
