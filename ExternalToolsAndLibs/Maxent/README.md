# A Maximum Entropy Modeling Toolkit for Python and C++

http://homepages.inf.ed.ac.uk/lzhang10/maxent.html

## What is this?
This package provides a (Conditional) Maximum Entropy Modeling Toolkit for
Python and C++. The library is written in ISO C++ and has been tested under
GNU/Linux, FreeBSD/NetBSD and Win32 (Cygwin and http://www.mingw.org).
There is also a python extension module (maxent module) available.

For more information, please have a look at the file manual.pdf in doc/
directory. The PDF document talks about the toolkit at length.

## Building
See file INSTALL for detail description of building the maxent package on unix
platforms. Instruction on building under win32 environment is covered in the
PDF manual in doc/.

python/ directory contains source code for python binding of the toolkit. The
detail instruction on building the python extension is given in python/README.

## License
This software is freeware and is released under LGPL. see LICENSE file for
more information. 

The adoption of LGPL is in accord with the license of java maxent project:
http://maxent.sf.net, from which the toolkit was derived.  LGPL makes it
easier to share source code, as well as new ideas, between both projects.

## Misc Info
### About Feature Value
Since binary feature and integer feature are more common than high precision
real feature, all feature values are represented as float (4 bytes) rather
than double (8 bytes) in order to save memory. If you need to specify double
feature values you can find all floats in source code and replace them with
double (require more memory).


# Contribute
Pull requests are welcome.
