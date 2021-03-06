//-*- C++ -*-
//-*- coding: utf-8 -*-
//
// Author: Joshua Cohen, Liang Yu, Bryan Riel
// Copyright 2017-2018

#include <algorithm>
#include "Interpolator.h"

/** @param[in] sincLen Length of sinc kernel
  * @param[in] sincSub Sinc decimation factor */
template <typename U>
isce3::core::Sinc2dInterpolator<U>::
Sinc2dInterpolator(int sincLen, int sincSub) :
    isce3::core::Interpolator<U>(SINC_METHOD),
    _kernelLength{sincSub}, _kernelWidth{sincLen}, _sincHalf{sincLen / 2} {

    // Temporary valarray for storing sinc coefficients
    std::valarray<double> filter(0.0, sincSub * sincLen);
    _sinc_coef(1.0, sincLen, sincSub, 0.0, 1, filter);

    // Resize member kernel matrix
    _kernel.resize(sincSub, sincLen);

    // Normalize filter
    for (size_t i = 0; i < sincSub; ++i) {
        // Compute filter sum
        double ssum = 0.0;
        for (size_t j = 0; j < sincLen; ++j) {
            ssum += filter[i + sincSub*j];
        }
        // Normalize the filter coefficients and copy to transposed member kernel
        for (size_t j = 0; j < sincLen; ++j) {
            filter[i + sincSub*j] /= ssum;
            _kernel(i,j) = filter[i + sincSub*j];
        }
    }
}

/** @param[in] x X-coordinate to interpolate
  * @param[in] y Y-coordinate to interpolate
  * @param[in] z 2D matrix to interpolate. */
template<class U>
U isce3::core::Sinc2dInterpolator<U>::interp_impl(double x, double y,
                                                 const Map& z) const
{

    // Separate interpolation coordinates into integer and fractional components
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const double fx = x - ix;
    const double fy = y - iy;

    // Check edge conditions
    U interpVal(0.0);
    if ((ix < (_sincHalf - 1)) || (ix > (z.cols() - _sincHalf - 1)))
        return interpVal;
    if ((iy < (_sincHalf - 1)) || (iy > (z.rows() - _sincHalf - 1)))
        return interpVal;

    // Modify integer interpolation coordinates for sinc evaluation
    const int xx = ix + _sincHalf;
    const int yy = iy + _sincHalf;

    // Call sinc interpolator
    interpVal = _sinc_eval_2d(z, xx, yy, fx, fy);
    return interpVal;
}

template<class U>
U isce3::core::Sinc2dInterpolator<U>::_sinc_eval_2d(const Map& arrin, int intpx,
                                                   int intpy, double frpx,
                                                   double frpy) const
{

    // Initialize return value
    U ret(0.0);
    
    // Get nearest kernel indices
    int ifracx = std::min(std::max(0, int(frpx*_kernelLength)), _kernelLength-1);
    int ifracy = std::min(std::max(0, int(frpy*_kernelLength)), _kernelLength-1);

    // Compute weighted sum from kernel
    for (int i = 0; i < _kernelWidth; i++) {
        for (int j = 0; j < _kernelWidth; j++) {
            ret += arrin(intpy-i,intpx-j) * 
                   static_cast<U>(_kernel(ifracy,i)) * 
                   static_cast<U>(_kernel(ifracx,j));
        }
    }

    // Done
    return ret;
}

template<class U>
void isce3::core::Sinc2dInterpolator<U>::_sinc_coef(
        double beta, double, int decfactor, double pedestal, int weight,
        std::valarray<double>& filter) const
{

    int filtercoef = int(filter.size());
    double wgthgt = (1.0 - pedestal) / 2.0;
    double soff = (filtercoef - 1.) / 2.;

    double wgt, s, fct;
    for (int i = 0; i < filtercoef; i++) {
        wgt = (1. - wgthgt) + (wgthgt * std::cos((M_PI * (i - soff)) / soff));
        s = (std::floor(i - soff) * beta) / (1. * decfactor);
        fct = ((s != 0.) ? (std::sin(M_PI * s) / (M_PI * s)) : 1.);
        filter[i] = ((weight == 1) ? (fct * wgt) : fct);
    }
}

// Forward declaration of classes
template class isce3::core::Sinc2dInterpolator<double>;
template class isce3::core::Sinc2dInterpolator<float>;
template class isce3::core::Sinc2dInterpolator<std::complex<double>>;
template class isce3::core::Sinc2dInterpolator<std::complex<float>>;

// end of file
