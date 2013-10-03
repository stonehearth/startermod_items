#ifndef _RADIANT_VOXEL_VOXEL_UTIL_H
#define _RADIANT_VOXEL_VOXEL_UTIL_H

#include "namespace.h"
#include "csg/point.h"
#include "csg/color.h"
#include "qubicle_file_format.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

Region3 MatrixToRegion3(QubicleMatrix const& matrix);

END_RADIANT_VOXEL_NAMESPACE


#endif //  _RADIANT_VOXEL_VOXEL_UTIL_H
