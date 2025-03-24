#ifdef __cplusplus
extern "C" {
#endif


#ifndef WMM_POINT_H
#define WMM_POINT_H
#include <stddef.h>
#include "GeomagnetismHeader.h"
//#include "EGM9615.h"

int omg_wmm(MAGtype_CoordGeodetic *CoordGeodeticArr,
            MAGtype_Date *UserDateArr,
            MAGtype_GeoMagneticElements *GeoMagneticElementsArr,
            MAGtype_GeoMagneticElements *ErrorsArr,
            int length);

int omg_emm(MAGtype_CoordGeodetic *CoordGeodeticArr,
            MAGtype_Date *UserDateArr,
            MAGtype_GeoMagneticElements *GeoMagneticElementsArr,
            MAGtype_GeoMagneticElements *ErrorsArr,
            int length);
int omg_wmm_hr(MAGtype_CoordGeodetic *CoordGeodeticArr,
            MAGtype_Date *UserDateArr,
            MAGtype_GeoMagneticElements *GeoMagneticElementsArr,
            MAGtype_GeoMagneticElements *ErrorsArr,
            int length);
#endif // WMM_POINT_H


#ifdef __cplusplus
}
#endif
