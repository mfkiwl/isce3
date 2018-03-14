//
// Author: Joshua Cohen
// Copyright 2017
//

#ifndef __ISCE_DATAIO_RASTERBLOCKITER_H__
#define __ISCE_DATAIO_RASTERBLOCKITER_H__

#include <cmath>
#include <vector>
#include "Raster.h"

namespace isce {
  namespace dataio {
    class RasterBlockIter {

    public:
        RasterBlockIter() = delete;
        RasterBlockIter(const RasterBlockIter &rli) : _raster(rli._raster), _xidx(rli._xidx),
                                                      _yidx(rli._yidx), _xskip(rli._xskip),
                                                      _yskip(rli._yskip) {}
        RasterBlockIter(const Raster &r) : _raster(r), _xidx(0), _yidx(0), _xskip(128), _yskip(128) {}
        RasterBlockIter(const Raster &r, const int x, const int y) : _raster(r),
	                                                        _xidx(0), _yidx(0), _xskip(x), _yskip(y) {}


	// Getters
	inline size_t xidx() const {return _xidx;}
	inline size_t yidx() const {return _xidx;}
	inline size_t xskip() const {return _xskip;}
	inline size_t yskip() const {return _yskip;}
	
        inline RasterBlockIter& operator=(const RasterBlockIter&);
        RasterBlockIter& operator+=(const size_t);
        RasterBlockIter& operator-=(const size_t);
        inline RasterBlockIter& operator+(const size_t rh) { return RasterBlockIter(*this) += rh; }
        inline RasterBlockIter& operator-(const size_t rh) { return RasterBlockIter(*this) -= rh; }
        // Prefix increment
        inline RasterBlockIter& operator++();
        // Postfix increment
        inline RasterBlockIter operator++(int);
        // Prefix decrement
        inline RasterBlockIter& operator--();
        // Postfix decrement
        inline RasterBlockIter operator--(int);
        inline bool operator==(const RasterBlockIter&);
        inline bool operator!=(const RasterBlockIter &rhs) { return !(*this == rhs); }

        inline void rewind();
        inline void ffwd();
        // Since the only way to have this _xidx is to increment to the end, no need to check _yidx
        inline bool atEOF() { return (_yidx == ceil((1. * _raster.length()) / yskip())); };
        inline RasterBlockIter atBeginning();
        inline RasterBlockIter atEnd();
    
        // (buffer, nXelem, nYelem, [band-index])
        template<typename T> void getNext(T*, size_t, size_t, size_t);
        template<typename T> void getNext(T*, size_t, size_t);
        template<typename T> void getNext(std::vector<T>&,size_t,size_t,size_t);
        template<typename T> void getNext(std::vector<T>&,size_t,size_t);
        // (buffer, [band-index])
        template<typename T> void getNext(std::vector<std::vector<T>>&,size_t);
        template<typename T> void getNext(std::vector<std::vector<T>>&);
        
        // (buffer, nXelem, nYelem, [band-index])
        template<typename T> void setNext(T*, size_t, size_t, size_t);
        template<typename T> void setNext(T*, size_t, size_t);
        template<typename T> void setNext(std::vector<T>&,size_t,size_t,size_t);
        template<typename T> void setNext(std::vector<T>&,size_t,size_t);
        // (buffer, [band-index])
        template<typename T> void setNext(std::vector<std::vector<T>>&,size_t);
        template<typename T> void setNext(std::vector<std::vector<T>>&);

    private:
	Raster _raster;
        // Idxs are the actual block numbers
        size_t _xidx;
        size_t _yidx;
        // Skips are used to determine block spacing (a "natural" block size is the skip size)
        size_t _xskip;
        size_t _yskip;

    };


    
    inline RasterBlockIter& RasterBlockIter::operator=(const RasterBlockIter &rhs) {
        _raster = rhs._raster;
        _xidx = rhs._xidx;
        _yidx = rhs._yidx;
        _xskip = rhs._xskip;
        _yskip = rhs._yskip;
        return *this;
    }

    inline RasterBlockIter& RasterBlockIter::operator++() {
        // The logic is a little complex, so leverage the += operator
        *this += 1;
        return *this;
    }

    inline RasterBlockIter RasterBlockIter::operator++(int) {
        RasterBlockIter old(*this);
        operator++();
        return old;
    }

    inline RasterBlockIter& RasterBlockIter::operator--() {
        // The logic is a little complex, so leverage the -= operator
        *this -= 1;
        return *this;
    }

    inline RasterBlockIter RasterBlockIter::operator--(int) {
        RasterBlockIter old(*this);
        operator--();
        return old;
    }

    inline bool RasterBlockIter::operator==(const RasterBlockIter &rhs) {
        return (_raster.dataset == rhs._raster.dataset) && (_xidx == rhs._xidx) && 
               (_yidx == rhs._yidx) && (_xskip == rhs._xskip) && (_yskip == rhs._yskip);
    }

    inline void RasterBlockIter::rewind() {
        _xidx = 0;
        _yidx = 0;
    }

    inline void RasterBlockIter::ffwd() {
        // Off-the-end block is the first block in the first nonvalid block-line in the image (the
        // index is simply the number of block-lines in the image)
      _xidx = ceil((1. * _raster.width()) / xskip());
      _yidx = 0;
    }

    inline RasterBlockIter RasterBlockIter::atBeginning() {
        // This and atEnd() are not truly begin()/end(), so slightly different naming scheme
        RasterBlockIter ret(*this);
        ret.rewind();
        return ret;
    }

    inline RasterBlockIter RasterBlockIter::atEnd() {
        RasterBlockIter ret(*this);
        ret.ffwd();
        return ret;
    }
}}

#define ISCE_DATAIO_RASTERBLOCKITER_ICC
#include "RasterBlockIter.icc"
#undef ISCE_DATAIO_RASTERBLOCKITER_ICC

#endif
