set(SRCS
container/container.cpp
container/RadarGeometry.cpp
core/core.cpp
core/Attitude.cpp
core/Basis.cpp
core/Constants.cpp
core/DateTime.cpp
core/Ellipsoid.cpp
core/EulerAngles.cpp
core/Interp1d.cpp
core/Kernels.cpp
core/Linspace.cpp
core/LookSide.cpp
core/LUT1d.cpp
core/LUT2d.cpp
core/Orbit.cpp
core/Quaternion.cpp
core/StateVector.cpp
core/TimeDelta.cpp
focus/Backproject.cpp
focus/Chirp.cpp
focus/DryTroposphereModel.cpp
focus/focus.cpp
focus/RangeComp.cpp
geocode/geocode.cpp
geocode/GeocodeSlc.cpp
geometry/boundingbox.cpp
geometry/DEMInterpolator.cpp
geocode/GeocodeCov.cpp
geometry/geometry.cpp
geometry/geo2rdr.cpp
geometry/rdr2geo.cpp
geometry/RTC.cpp
image/image.cpp
image/ResampSlc.cpp
io/gdal/Dataset.cpp
io/gdal/GDALAccess.cpp
io/gdal/GDALDataType.cpp
io/gdal/gdal.cpp
io/gdal/Raster.cpp
io/Raster.cpp
io/serialization.cpp
io/io.cpp
signal/signal.cpp
signal/Covariance.cpp
signal/Crossmul.cpp
product/GeoGridParameters.cpp
product/product.cpp
product/RadarGridParameters.cpp
product/Swath.cpp
unwrap/unwrap.cpp
unwrap/ICU.cpp
isce.cpp
)

if(WITH_CUDA)
    list(APPEND SRCS
         cuda/cuda.cpp
         cuda/core/core.cpp
         cuda/core/ComputeCapability.cpp
         cuda/core/Device.cpp
         cuda/geometry/geometry.cpp
         cuda/geometry/geo2rdr.cpp
         cuda/geometry/rdr2geo.cpp
         cuda/focus/Backproject.cpp
         cuda/focus/focus.cpp
         cuda/image/image.cpp
         cuda/image/ResampSlc.cpp
         cuda/signal/signal.cpp
         cuda/signal/Crossmul.cpp
         )
endif()
