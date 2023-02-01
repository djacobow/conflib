# conflib

Very small simple "map" implementation for storing config data on low-resource devices

## Why?

Sometimes we want to store configuration data for a device, perhaps in NV storage
in a way that is semi-structured, but that does not require malloc, and in a form
which is _very_ compact.

## What

This library implements a simple key/value store where the keys are 8b numbers
and the values are unsigned numbers up to 32b. Everything is stored linearly
in a single simple buffer. Lookups are performed by scanning. Addition is done
by appending, or sometimes updating in place. Removals are done by findind the
item to be removed and memmoving everything past it.

### Details

The format of an entry is `<addr><size><data>` where the address is an 8b
value that is not zero. Size is a 3b value that indicates that the data is:
0b, 1b, 2b, 4b, 8b, 16b, or 32b. Data is the number of bits promised by size.

These blobs therefore vary in size from 11b total to 43b total. They are simply
concatted in order. The end of the line is indicated  by 8b of 0's, that is, an
implicit "zero address".


There are two implementation here, one in C, one in Python. The are compatible.

There is a `main.cpp` that includes some basic tests of the C implementation,
and `conflib.py` contains a basic test of you run it rather than importing.


