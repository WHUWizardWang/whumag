#ifdef __cplusplus
extern "C" {
#endif

#ifndef IGRF_POINT_H
#define IGRF_POINT_H

#include "GeomagnetismHeader.h"

int omg_igrf(MAGtype_CoordGeodetic *CoordGeodeticArr,
             MAGtype_Date *UserDateArr,
             MAGtype_GeoMagneticElements *GeoMagneticElementsArr,
             MAGtype_GeoMagneticElements *ErrorsArr,
             int length);

#endif // IGRF_POINT_H


#ifdef __cplusplus
}
#endif
