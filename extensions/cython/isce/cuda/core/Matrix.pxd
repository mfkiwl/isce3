#cython: language_level=3
#
# Author: Bryan V. Riel
# Copyright: 2017-2018
#

# Valarray
cdef extern from "<valarray>" namespace "std":
    cdef cppclass valarray[T]:
        # Constructors
        valarray()
        valarray(int)
        # Access element
        T & operator[](int)

# Matrix
cdef extern from "isce/core/Interpolator.h" namespace "isce::core":
    cdef cppclass Matrix[T]:
        # Constructors
        Matrix() except +
        Matrix(size_t length, size_t width) except +
        # Functions
        void resize(size_t length, size_t width)
        # Access element
        T & operator()(size_t row, size_t column)
        # Get reference to valarray data
        valarray[T] & data()
 
# end of file 