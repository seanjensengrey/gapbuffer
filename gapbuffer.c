/*
Copyright (c) 2007 Neil Hodgson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
A Python extension that provides the GapBuffer type, a mutable sequence of
characters, Unicode characters or integers. It implements Python's buffer and
sequence protocols.
*/

#include <Python.h>
#include "structmember.h"

typedef struct {
	PyObject_HEAD
	/* Type-specific fields go here. */
	char *body;
	int size;
	int lengthBody;
	int part1Length;
	int gapLength;	/// invariant: gapLength == size - lengthBody
	int growSize;
	int itemSize;
	int bufferAppearence;
	char itemType;
	int lock;
}
GapBuffer;

static char *
_GapBuffer_at(GapBuffer* self, int position) {
	if (position < self->part1Length) {
		return self->body + position;
	} else {
		return self->body + self->gapLength + position;
	}
}

static PyTypeObject gapbuffer_GapBufferType;

static void
GapBuffer_dealloc(GapBuffer* self) {
	PyMem_Del(self->body);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static void
_GapBuffer_InitFields(GapBuffer *self) {
	if (self != NULL) {
		self->body = NULL;
		self->growSize = 8;
		self->size = 0;
		self->lengthBody = 0;
		self->part1Length = 0;
		self->gapLength = 0;
		self->itemSize = 1;
		self->itemType = 'c';
		self->bufferAppearence = 0;
		self->lock = 0;
	}
}

static PyObject *
GapBuffer_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	GapBuffer *self;

	self = (GapBuffer *)type->tp_alloc(type, 0);

	_GapBuffer_InitFields(self);

	return (PyObject *)self;
}

static void _GapBuffer_GapTo(GapBuffer *self, int position) {
	if (position != self->part1Length) {
		if (position < self->part1Length) {
			memmove(
			    self->body + position + self->gapLength,
			    self->body + position,
			    self->part1Length - position);
		} else {	// position > part1Length
			memmove(
			    self->body + self->part1Length,
			    self->body + self->part1Length + self->gapLength,
			    position - self->part1Length);
		}
		self->part1Length = position;
	}
}

static void _GapBuffer_ReAllocate(GapBuffer *self, int newSize) {
	// Move the gap to the end
	char *newBody = NULL;
	_GapBuffer_GapTo(self, self->lengthBody);
	newBody = PyMem_New(char, newSize);
	if ((self->size != 0) && (self->body != NULL)) {
		memmove(newBody, self->body, self->lengthBody);
		PyMem_Del(self->body);
	}
	self->body = newBody;
	self->gapLength += newSize - self->size;
	self->size = newSize;
}

static void _GapBuffer_RoomFor(GapBuffer *self, int insertionLength) {
	if (self->gapLength <= insertionLength) {
		if (self->growSize * 6 < self->size)
			self->growSize *= 2;
		_GapBuffer_ReAllocate(self, self->size + insertionLength + self->growSize);
	}
}

static void
_GapBuffer_insertarray(GapBuffer* self, int position, const char *text, int insertLength) {
	_GapBuffer_RoomFor(self, insertLength);
	_GapBuffer_GapTo(self, position);
	memmove(self->body + self->part1Length, text, insertLength);
	self->lengthBody += insertLength;
	self->part1Length += insertLength;
	self->gapLength -= insertLength;
}

static int
_GapBuffer_insertiter(GapBuffer* self, int position, PyObject *sequence) {
	PyObject *value;
	PyObject *iter;
	iter = PyObject_GetIter(sequence);
	if (iter == NULL) {
		PyErr_SetString(PyExc_TypeError, "GapBuffer: argument not iterable");
		return 1;
	}
	while ((value = PyIter_Next(iter)) != NULL) {
		int ival = PyLong_AsLong(value);
		if ((ival == -1) && PyErr_Occurred()) {
			PyErr_SetString(PyExc_TypeError, "GapBuffer: argument wrong type");
			return 1;
		}
		_GapBuffer_insertarray(self, position * self->itemSize, (const char *)&ival, self->itemSize);
		Py_DECREF(value);
		position++;
	}
	Py_DECREF(iter);
	return 0;
}

static int
GapBuffer_init(GapBuffer *self, PyObject *args, PyObject *kwds) {
	PyObject *value;

	_GapBuffer_InitFields(self);

	if (!PyArg_ParseTuple(args, "|O:GapBuffer", &value)) {
		return -1;
	}

	if (value && PyUnicode_Check(value)) {
		self->itemType = 'u';
		self->itemSize = sizeof(Py_UNICODE);
		_GapBuffer_insertarray(self, 0, (char *)PyUnicode_AS_UNICODE(value),
		        PyUnicode_GET_SIZE(value) * self->itemSize);
	} else if (!value || PyBytes_Check(value)) {
		self->itemType = 'c';
		self->itemSize = 1;
		if (value) {
			_GapBuffer_insertarray(self, 0, PyBytes_AS_STRING(value),
			        PyBytes_GET_SIZE(value));
		}
	} else {
		// Assume iterable
		self->itemType = 'i';
		self->itemSize = sizeof(int);
		_GapBuffer_insertiter(self, 0, value);
	}

	return 0;
}

// TODO stop exposing these - they are only for debugging
static PyMemberDef GapBuffer_members[] = {
            {"size", T_INT, offsetof(GapBuffer, size), READONLY, "Allocated size"},
            {"part1Length", T_INT, offsetof(GapBuffer, part1Length), READONLY, "Length before gap"},
            {"gapLength", T_INT, offsetof(GapBuffer, gapLength), READONLY, "Length of gap"},
            {"growSize", T_INT, offsetof(GapBuffer, growSize), READONLY, "Size to grow"},
            {"itemsize", T_INT, offsetof(GapBuffer, itemSize), READONLY, "Size of each item"},
            {"typecode", T_CHAR, offsetof(GapBuffer, itemType), READONLY, "Type code of each item"},
            {"bufferAppearence", T_INT, offsetof(GapBuffer, bufferAppearence), 0, "Single or multiple segments"},
            {NULL}  /* Sentinel */
        };

static PyObject *
GapBuffer_insert(GapBuffer* self, PyObject *args) {
	char *data;
	int positionToInsert;
	Py_ssize_t insertLength;
	PyObject *sequence = NULL;

	if (self->lock) {
		PyErr_SetString(PyExc_BufferError, "Object is locked.");
		return NULL;
	}

	if (self->itemType == 'c') {
		if (!PyArg_ParseTuple(args, "is#:insert", &positionToInsert, &data, &insertLength)) {
			return NULL;
		}
	} else if (self->itemType == 'u') {
		if (!PyArg_ParseTuple(args, "iu#:insert", &positionToInsert, &data, &insertLength)) {
			return NULL;
		}
	} else {    // self->itemType == 'i'
		if (!PyArg_ParseTuple(args, "iO:insert", &positionToInsert, &sequence)) {
			return NULL;
		}
	}

	positionToInsert *= self->itemSize;
	if ((positionToInsert < 0) || (positionToInsert > self->lengthBody)) {
		PyErr_SetString(PyExc_IndexError, "GapBuffer.insert(position, text): out of range");
		return NULL;
	}

	if (sequence) {
		if (0 != _GapBuffer_insertiter(self, positionToInsert / self->itemSize, sequence)) {
			return NULL;
		}
	} else {
		_GapBuffer_insertarray(self, positionToInsert, data, insertLength * self->itemSize);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
GapBuffer_extend(GapBuffer* self, PyObject *args) {
	char *data;
	Py_ssize_t insertLength;
	PyObject *sequence = NULL;

	if (self->itemType == 'c') {
		if (!PyArg_ParseTuple(args, "s#:extend", &data, &insertLength)) {
			return NULL;
		}
	} else if (self->itemType == 'u') {
		if (!PyArg_ParseTuple(args, "u#:extend", &data, &insertLength)) {
			return NULL;
		}
	} else {    // self->itemType == 'i'
		if (!PyArg_ParseTuple(args, "O:extend", &sequence)) {
			return NULL;
		}
	}

	if (sequence) {
		if (0 != _GapBuffer_insertiter(self, self->lengthBody / self->itemSize, sequence)) {
			return NULL;
		}
	} else {
		_GapBuffer_insertarray(self, self->lengthBody, data, insertLength * self->itemSize);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static void
memincr1(char *p, int length, int v) {
	while (length-- > 0) {
		*p++ += v;
	}
}

static void
memincr2(short *p, int length, int v) {
	while (length-- > 0) {
		*p++ += v;
	}
}

static void
memincr4(int *p, int length, int v) {
	while (length-- > 0) {
		*p++ += v;
	}
}

static PyObject *
GapBuffer_increment(GapBuffer* self, PyObject *args) {
	int position;
	int length;
	int value;
	int lengthInPart1;
	int lengthInPart2;
	char *positionInPart2;

	if (!PyArg_ParseTuple(args, "iii:increment", &position, &length, &value)) {
		return NULL;
	}

	if ((position < 0) || ((position + length) > self->lengthBody / self->itemSize)) {
		PyErr_SetString(PyExc_IndexError, "GapBuffer.increment(position, length, value): out of range");
		return NULL;
	}

	position *= self->itemSize;
	length *= self->itemSize;
	lengthInPart1 = self->part1Length - position;
	if (lengthInPart1 > length)
		lengthInPart1 = length;
	else if (lengthInPart1 < 0)
		lengthInPart1 = 0;
	if (lengthInPart1 > 0)
		positionInPart2 = self->body + self->part1Length + self->gapLength;
	else
		positionInPart2 = self->body + self->gapLength + position;
	lengthInPart2 = (length - lengthInPart1) / self->itemSize;
	lengthInPart1 /= self->itemSize;

	switch (self->itemSize) {
	case 1:
		memincr1(self->body + position, lengthInPart1, value);
		memincr1(positionInPart2, lengthInPart2, value);
		break;
	case 2:
		memincr2((short *)(self->body + position), lengthInPart1, value);
		memincr2((short *)(positionInPart2), lengthInPart2, value);
		break;
	case 4:
		memincr4((int *)(self->body + position), lengthInPart1, value);
		memincr4((int *)(positionInPart2), lengthInPart2, value);
		break;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
_GapBuffer_retrieve(GapBuffer* self, int positionToRetrieve, int retrieveLength) {
	PyObject* retrievedString = NULL;
	char *retStrPtr = NULL;
	int i = 0;

	positionToRetrieve *= self->itemSize;
	retrieveLength *= self->itemSize;

	if ((positionToRetrieve < 0) || ((positionToRetrieve + retrieveLength) > self->lengthBody)) {
		PyErr_SetString(PyExc_IndexError, "GapBuffer.retrieve(position, length): out of range");
		return NULL;
	}

	if (self->itemType == 'c') {
		retrievedString = PyBytes_FromStringAndSize(NULL, retrieveLength);
		if (retrievedString == 0)
			return 0;
		retStrPtr = (char *)PyBytes_AsString(retrievedString);
	} else if (self->itemType == 'u') {
		retrievedString = PyUnicode_FromUnicode(NULL, retrieveLength / self->itemSize);
		if (retrievedString == 0)
			return 0;
		retStrPtr = (char *)PyUnicode_AS_UNICODE(retrievedString);
	} else {    // self->itemType == 'i'
		PyErr_SetString(PyExc_TypeError, "GapBuffer.retrieve(position, length): wrong type");
		return NULL;
	}

	while ((i < retrieveLength) && (positionToRetrieve < self->part1Length)) {
		retStrPtr[i++] = self->body[positionToRetrieve++];
	}
	while (i < retrieveLength) {
		retStrPtr[i++] = self->body[self->gapLength + positionToRetrieve++];
	}

	return retrievedString;
}

static PyObject *
GapBuffer_retrieve(GapBuffer* self, PyObject *args) {
	int positionToRetrieve = 0;
	int retrieveLength = 0;

	if (!PyArg_ParseTuple(args, "ii:retrieve", &positionToRetrieve, &retrieveLength)) {
		return NULL;
	}

	return _GapBuffer_retrieve(self, positionToRetrieve, retrieveLength);
}

static int
GapBuffer_compare(GapBuffer* self, PyObject *other) {
	GapBuffer *o;
	int i;

	if (!PyObject_TypeCheck(other, &gapbuffer_GapBufferType)) {
		PyErr_SetString(PyExc_TypeError, "GapBuffer compare: wrong type");
		return -1;
	}
	o = (GapBuffer *)other;
	if (self->itemType != o->itemType) {
		return (self->itemType > o->itemType) ? 1 : -1;
	}
	for (i = 0; i < self->lengthBody; i++) {
		if (i >= o->lengthBody)
			return 1;
		if (*_GapBuffer_at(self, i) != *_GapBuffer_at(o, i)) {
			return (*_GapBuffer_at(self, i) > *_GapBuffer_at(o, i)) ? 1 : -1;
		}
	}
	//~ printf("} compare %d\n", i); fflush(stdout);
	if (self->lengthBody < o->lengthBody)
		return -1;
	return 0;
}

static PyObject *
GapBuffer_richcmp(PyObject *obj1, PyObject *obj2, int op) {
	int result = 0;
	PyObject *ret;

	if (!PyObject_TypeCheck(obj2, &gapbuffer_GapBufferType)) {
		PyErr_SetString(PyExc_TypeError, "GapBuffer compare: wrong type");
		if (op == Py_NE)
			result = 1;
	} else {
		int relation = GapBuffer_compare((GapBuffer *)obj1, obj2);
		switch (op) {
		case Py_LT: result = relation <  0; break;
		case Py_LE: result = relation <= 0; break;
		case Py_EQ: result = relation == 0; break;
		case Py_NE: result = relation != 0; break;
		case Py_GT: result = relation >  0; break;
		case Py_GE: result = relation >= 0; break;
		}
	}
	ret = result ? Py_True : Py_False;
	Py_INCREF(ret);
	return ret;
}

static PyObject *
GapBuffer_str(GapBuffer* self) {
	if (self->itemType == 'c') {
#if PY_MAJOR_VERSION >= 3
		PyObject *pbytes = _GapBuffer_retrieve(self, 0, self->lengthBody);
		PyObject *repr = PyBytes_Repr(pbytes, 0);
		Py_DECREF(pbytes);
		return repr;
#else
		return _GapBuffer_retrieve(self, 0, self->lengthBody);
#endif
	} else if (self->itemType == 'u') {
		return _GapBuffer_retrieve(self, 0, self->lengthBody / self->itemSize);
	} else {
		char buf[1024];
		char elem[256];
		int i;
		int elements = self->lengthBody / self->itemSize;
		int maxElements = 10;
		PyOS_snprintf(buf, sizeof(buf), "GapBuffer('%c') [", self->itemType);
		for (i = 0; i < maxElements && i < elements; i++) {
			PyOS_snprintf(elem, sizeof(elem), "%d, ", *(int *)_GapBuffer_at(self, i * self->itemSize));
			strcat(buf, elem);
		}
		if (elements > maxElements) {
			strcat(buf, "...]");
		} else {
			buf[strlen(buf)-2] = '\0';
			strcat(buf, "]");
		}
#if PY_MAJOR_VERSION >= 3
		return PyUnicode_FromString(buf);
#else
		return PyString_FromString(buf);
#endif
	}
}

// Minimize memory used
static PyObject *
GapBuffer_slim(GapBuffer *self) {
	if (self->lock) {
		PyErr_SetString(PyExc_BufferError, "Object is locked.");
		Py_INCREF(Py_None);
		return Py_None;
	}
	// Reduce growSize
	while ((self->growSize > 8) && (self->growSize * 3 > self->lengthBody))
		self->growSize /= 2;
	_GapBuffer_ReAllocate(self, self->lengthBody / 8 * 8 + self->growSize);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef GapBuffer_methods[] = {
            {"retrieve", (PyCFunction)GapBuffer_retrieve, METH_VARARGS, "Retrieve a portion as a string"	},
            {"insert", (PyCFunction)GapBuffer_insert, METH_VARARGS, "Insert a string" },
            {"extend", (PyCFunction)GapBuffer_extend, METH_VARARGS, "Extend with a string" },
            {"increment", (PyCFunction)GapBuffer_increment, METH_VARARGS, "Increment a range of values" },
            {"slim", (PyCFunction)GapBuffer_slim, METH_VARARGS, "Minimize memory used" },
            {NULL}  /* Sentinel */
        };

#if PY_MAJOR_VERSION >= 3

static int GapBuffer_getbufferproc(GapBuffer *self, Py_buffer *view, int flags) {
	// Move gap to end so bytes are contiguous
	_GapBuffer_GapTo(self, self->lengthBody);

	Py_INCREF(self);
	view->obj = (PyObject*)self;
	view->buf = self->body;
	view->len = self->lengthBody;
	view->readonly = 0;
	if (flags & PyBUF_FORMAT) {
		if (self->itemType == 'c') {
			view->format = "c";
		} else if (self->itemType == 'u') {
			view->format = "s";
		} else {    // self->itemType == 'i'
			view->format = "i";
		}
	} else {
		view->format = "B";
	}
	view->ndim = 1;
	view->strides = NULL;
	view->suboffsets = NULL;
	view->itemsize = self->itemSize;
	view->internal = 0;
	self->lock++;
	return 0;
}

static void GapBuffer_releasebufferproc(GapBuffer *self, Py_buffer *view) {
	self->lock--;
}

static PyBufferProcs GapBuffer_bufferprocs = {
            (getbufferproc)GapBuffer_getbufferproc,
            (releasebufferproc)GapBuffer_releasebufferproc,
        };

#else

// Python 2.x buffer procs

// The getreadbufferproc, getwritebufferproc, and getcharbufferproc are mostly the same
int _GapBuffer_getbufferproc(GapBuffer *self, Py_ssize_t index, const void **ptr) {
	if (self->bufferAppearence == 0) {
		_GapBuffer_GapTo(self, self->lengthBody);
		*ptr = self->body;
		return self->lengthBody;
	} else {
		if (index != 0 && index != 1) {
			PyErr_SetString(PyExc_SystemError,
			        "Accessing non-existent gap buffer segment");
			return -1;
		}
		if (index == 0) {
			*ptr = self->body;
			return self->part1Length;
		} else {
			*ptr = self->body + self->part1Length + self->gapLength;
			return self->lengthBody - self->part1Length;
		}
	}
}

int GapBuffer_getreadbufferproc(GapBuffer *self, Py_ssize_t index, const void **ptr) {
	return _GapBuffer_getbufferproc(self, index, ptr);
}

int GapBuffer_getwritebufferproc(GapBuffer *self, Py_ssize_t index, const void **ptr) {
	return _GapBuffer_getbufferproc(self, index, ptr);
}

int GapBuffer_getsegcountproc(GapBuffer *self, int *lenp) {
	if ( lenp )
		*lenp = self->lengthBody;
	if (self->bufferAppearence == 0) {
		return 1;
	} else {
		return 2;
	}
}

int GapBuffer_getcharbufferproc(GapBuffer *self, Py_ssize_t index, const void **ptr) {
	if (self->itemType != 'c') {
		PyErr_SetString(PyExc_TypeError, "GapBuffer not of char type");
		return -1;
	}
	return _GapBuffer_getbufferproc(self, index, ptr);
}

static PyBufferProcs GapBuffer_bufferprocs = {
            (readbufferproc)GapBuffer_getreadbufferproc,
            (writebufferproc)GapBuffer_getwritebufferproc,
            (segcountproc)GapBuffer_getsegcountproc,
            (charbufferproc)GapBuffer_getcharbufferproc,
        };

#endif

static Py_ssize_t
GapBuffer_length(GapBuffer *self) {
	return self->lengthBody / self->itemSize;
}

static PyObject *
GapBuffer_item(GapBuffer *self, Py_ssize_t position) {
	char *ptr;

	position *= self->itemSize;

	if ((position < 0) || ((position + 1) > self->lengthBody)) {
		PyErr_SetString(PyExc_IndexError, "GapBuffer index out of range");
		return NULL;
	}

	ptr = _GapBuffer_at(self, position);
	if (self->itemType == 'c') {
		return PyBytes_FromStringAndSize(ptr, 1);
	} else if (self->itemType == 'u') {
		return PyUnicode_FromUnicode((Py_UNICODE *)(ptr), 1);
	} else {
		return PyLong_FromLong((long)*((int *)ptr));
	}
}

static PyObject *
GapBuffer_slice(GapBuffer *self, Py_ssize_t ilow, Py_ssize_t ihigh) {
	GapBuffer *nsv;
	int length;
	int i = 0;
	int position = 0;
	
	ilow *= self->itemSize;
	if (ihigh > self->lengthBody / self->itemSize)
		ihigh = self->lengthBody / self->itemSize;
	ihigh *= self->itemSize;

	if (ilow < 0)
		ilow = 0;
	else if (ilow > self->lengthBody)
		ilow = self->lengthBody;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > self->lengthBody)
		ihigh = self->lengthBody;
	nsv = (GapBuffer *) GapBuffer_new(&gapbuffer_GapBufferType, NULL, NULL);
	if (nsv == NULL)
		return NULL;
	nsv->itemType = self->itemType;
	nsv->itemSize = self->itemSize;
	length = ihigh - ilow;

	_GapBuffer_RoomFor(nsv, length);
	_GapBuffer_GapTo(nsv, 0);
	position = ilow;
	while ((i < length) && (position < self->part1Length)) {
		nsv->body[i++] = self->body[position++];
	}
	while (i < length) {
		nsv->body[i++] = self->body[self->gapLength + position++];
	}
	nsv->lengthBody += length;
	nsv->part1Length += length;
	nsv->gapLength -= length;

	return (PyObject *)nsv;
}

static PyObject *
GapBuffer_concat(GapBuffer *self, PyObject *other) {
	GapBuffer *o;
	GapBuffer *nsv;
	int lengthTotal;
	if (!PyObject_TypeCheck(other, &gapbuffer_GapBufferType)) {
		PyErr_SetString(PyExc_TypeError, "GapBuffer concat: must be GapBuffer");
		return NULL;
	}
	o = (GapBuffer *)other;
	if (self->itemType != o->itemType) {
		PyErr_SetString(PyExc_TypeError, "GapBuffer concat: different types");
		return NULL;
	}
	nsv = (GapBuffer *) GapBuffer_new(&gapbuffer_GapBufferType, NULL, NULL);
	if (nsv == NULL)
		return NULL;
	nsv->itemType = self->itemType;
	nsv->itemSize = self->itemSize;
	lengthTotal = self->lengthBody + o->lengthBody;
	_GapBuffer_RoomFor(nsv, lengthTotal);
	_GapBuffer_GapTo(nsv, 0);
	_GapBuffer_GapTo(self, self->lengthBody);
	_GapBuffer_GapTo(o, o->lengthBody);
	memmove(nsv->body, self->body, self->lengthBody);
	memmove(nsv->body + self->lengthBody, o->body, o->lengthBody);
	nsv->lengthBody += lengthTotal;
	nsv->part1Length += lengthTotal;
	nsv->gapLength -= lengthTotal;
	return (PyObject *)nsv;
}

static PyObject *
GapBuffer_repeat(GapBuffer *self, Py_ssize_t n) {
	GapBuffer *nsv;
	int lengthTotal;
	int i;
	nsv = (GapBuffer *) GapBuffer_new(&gapbuffer_GapBufferType, NULL, NULL);
	if (nsv == NULL)
		return NULL;
	nsv->itemType = self->itemType;
	nsv->itemSize = self->itemSize;
	lengthTotal = self->lengthBody * n;
	_GapBuffer_RoomFor(nsv, lengthTotal);
	_GapBuffer_GapTo(nsv, 0);
	_GapBuffer_GapTo(self, self->lengthBody);
	for (i = 0;i < n;i++) {
		memmove(nsv->body + self->lengthBody * i, self->body, self->lengthBody);
	}
	nsv->lengthBody += lengthTotal;
	nsv->part1Length += lengthTotal;
	nsv->gapLength -= lengthTotal;
	return (PyObject *)nsv;
}

static void
_GapBuffer_delete(GapBuffer *self, int position, int size) {
	_GapBuffer_GapTo(self, position);
	self->lengthBody -= size;
	self->gapLength += size;
}

static int
GapBuffer_ass_slice(GapBuffer *self, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v) {
	char *text = NULL;
	int insertLength = 0;
	GapBuffer *psv = NULL;

	if (self->lock) {
		PyErr_SetString(PyExc_BufferError, "Object is locked.");
		return -1;
	}
	ilow *= self->itemSize;
	if (ihigh == -1 || ihigh > (self->lengthBody / self->itemSize)) {
		ihigh = self->lengthBody;
	} else {
		ihigh *= self->itemSize;
	}

	if (ilow < 0)
		ilow = 0;
	else if (ilow > self->lengthBody)
		ilow = self->lengthBody;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > self->lengthBody)
		ihigh = self->lengthBody;
	_GapBuffer_delete(self, ilow, ihigh - ilow);

	if (v) {
		psv = (GapBuffer *)v;
		if (PyObject_TypeCheck(v, &gapbuffer_GapBufferType) &&
		        (psv->itemType == self->itemType)) {
			_GapBuffer_GapTo(psv, psv->lengthBody);
			text = psv->body;
			insertLength = psv->lengthBody / self->itemSize;
		} else if (self->itemType == 'c') {
			if (PyBytes_Check(v)) {
				text = PyBytes_AsString(v);
				insertLength = PyBytes_Size(v);
			} else {
				PyErr_SetString(PyExc_TypeError, "GapBuffer assign slice: wrong type");
				return -1;
			}
		} else if (self->itemType == 'u') {
			if (PyUnicode_Check(v)) {
				text = (char *)PyUnicode_AS_UNICODE(v);
				insertLength = PyUnicode_GET_SIZE(v);
			} else {
				PyErr_SetString(PyExc_TypeError, "GapBuffer assign slice: wrong type");
				return -1;
			}
		} else {
			if (0 != _GapBuffer_insertiter(self, ilow / self->itemSize, v)) {
				return -1;
			}
			return 0;
		}
	}

	_GapBuffer_insertarray(self, ilow, text, insertLength * self->itemSize);

	return 0;
}

static int
GapBuffer_ass_item(GapBuffer *self, Py_ssize_t position, PyObject *v) {
	if (self->lock) {
		PyErr_SetString(PyExc_BufferError, "Object is locked.");
		return -1;
	}
	if (self->itemType == 'i') {
		char *ptr;
		position *= self->itemSize;
		if ((position < 0) || ((position + 1) > self->lengthBody)) {
			PyErr_SetString(PyExc_IndexError, "GapBuffer index out of range");
			return -1;
		}
		if (v) {
			ptr = _GapBuffer_at(self, position);
			*((int *)ptr) = PyLong_AsLong(v);
		} else {
			// Deleting an item
			_GapBuffer_delete(self, position, self->itemSize);
		}
		return 0;
	} else {
		return GapBuffer_ass_slice(self, position, position + 1, v);
	}
}

static PySequenceMethods GapBuffer_as_sequence = {
            (lenfunc)GapBuffer_length,		       /*sq_length*/
            (binaryfunc)GapBuffer_concat,	       /*sq_concat*/
            (ssizeargfunc)GapBuffer_repeat,	       /*sq_repeat*/
            (ssizeargfunc)GapBuffer_item,		       /*sq_item*/
            (ssizessizeargfunc)GapBuffer_slice,	       /*sq_slice*/
            (ssizeobjargproc)GapBuffer_ass_item,	       /*sq_ass_item*/
            (ssizessizeobjargproc)GapBuffer_ass_slice,      /*sq_ass_slice*/
        };

static PyObject *GapBuffer_subscript(GapBuffer *self, PyObject *item) {
	if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength;
		if (PySlice_GetIndicesEx((PySliceObject*)item, 
			self->lengthBody, &start, &stop, 
			&step, &slicelength) < 0)
			return NULL;

		if (step != 1) {
			PyErr_SetString(PyExc_TypeError, "slice steps not supported");
			return NULL;
		}
		return GapBuffer_slice(self, start, stop);
	} else {
		int index = PyLong_AsLong(item);
		return GapBuffer_item(self, index);
	}
}

static int GapBuffer_ass_subscript(GapBuffer *self, PyObject *item, PyObject *value) {
	if (self->lock) {
		PyErr_SetString(PyExc_BufferError, "Object is locked.");
		return -1;
	}
	if (PySlice_Check(item)) {
		Py_ssize_t start, stop, step, slicelength;
		if (PySlice_GetIndicesEx((PySliceObject*)item, 
			self->lengthBody, &start, &stop, 
			&step, &slicelength) < 0)
			return 0;
		if (step != 1) {
			PyErr_SetString(PyExc_TypeError, "slice steps not supported");
			return 0;
		}
		GapBuffer_ass_slice(self, start, stop, value);
	} else {
		int index = PyLong_AsLong(item);
		GapBuffer_ass_item(self, index, value);
	}
	return 0;
}

static PyMappingMethods GapBuffer_as_mapping = {
            (lenfunc)GapBuffer_length,
            (binaryfunc)GapBuffer_subscript,
            (objobjargproc)GapBuffer_ass_subscript,
        };

static PyTypeObject gapbuffer_GapBufferType = {
            PyObject_HEAD_INIT(NULL)
#if PY_MAJOR_VERSION < 3
            0,                         /*ob_size*/
#endif
            "gapbuffer.GapBuffer",             /*tp_name*/
            sizeof(GapBuffer), /*tp_basicsize*/
            0,                         /*tp_itemsize*/
            (destructor)GapBuffer_dealloc,                         /*tp_dealloc*/
            0,                         /*tp_print*/
            0,                         /*tp_getattr*/
            0,                         /*tp_setattr*/
#if PY_MAJOR_VERSION >= 3
            0,                         /* was tp_compare*/
#else
            (cmpfunc)GapBuffer_compare,                         /*tp_compare*/
#endif
            (reprfunc)GapBuffer_str,                         /*tp_repr*/
            0,                         /*tp_as_number*/
            &GapBuffer_as_sequence,                         /*tp_as_sequence*/
            &GapBuffer_as_mapping,     /*tp_as_mapping*/
            0,                         /*tp_hash */
            0,                         /*tp_call*/
            0,                         /*tp_str*/
            0,                         /*tp_getattro*/
            0,                         /*tp_setattro*/
            &GapBuffer_bufferprocs,                         /*tp_as_buffer*/
#if PY_MAJOR_VERSION >= 3
            Py_TPFLAGS_DEFAULT,        /*tp_flags*/
#else
            Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GETCHARBUFFER,        /*tp_flags*/
#endif
            "GapBuffer objects",           /* tp_doc */
            0,		               /* tp_traverse */
            0,		               /* tp_clear */
            GapBuffer_richcmp,         /* tp_richcompare */
            0,		               /* tp_weaklistoffset */
            0,		               /* tp_iter */
            0,		               /* tp_iternext */
            GapBuffer_methods,             /* tp_methods */
            GapBuffer_members,             /* tp_members */
            0,                         /* tp_getset */
            0,                         /* tp_base */
            0,                         /* tp_dict */
            0,                         /* tp_descr_get */
            0,                         /* tp_descr_set */
            0,                         /* tp_dictoffset */
            (initproc)GapBuffer_init,      /* tp_init */
            PyType_GenericAlloc,                         /* tp_alloc */
            GapBuffer_new,                 /* tp_new */
        };

static PyMethodDef gapbuffer_methods[] = {
            {NULL}  /* Sentinel */
        };

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

#if PY_MAJOR_VERSION >= 3

static PyModuleDef gapbuffermodule = {
	PyModuleDef_HEAD_INIT,
	"gapbuffer",
	"Gap buffer extension type.",
	-1,
	NULL, NULL, NULL, NULL, NULL
};

#endif

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit_gapbuffer(void)
#else
initgapbuffer(void) 
#endif 
{
	PyObject *module = NULL;

	gapbuffer_GapBufferType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&gapbuffer_GapBufferType) >= 0) {

//__debugbreak();

#if PY_MAJOR_VERSION >= 3
		module = PyModule_Create(&gapbuffermodule);
#else
		module = Py_InitModule3("gapbuffer", gapbuffer_methods,
			"Gap buffer extension type.");
#endif

		if (module) {
			Py_INCREF(&gapbuffer_GapBufferType);
			PyModule_AddObject(module, "GapBuffer", (PyObject *)&gapbuffer_GapBufferType);
		}
	}
#if PY_MAJOR_VERSION >= 3
	return module;
#endif
}


