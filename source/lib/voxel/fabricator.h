#ifndef _RADIANT_VOXEL_QUBICLE_FABRICATOR_H
#define _RADIANT_VOXEL_QUBICLE_FABRICATOR_H

#include "namespace.h"
#include "csg/point.h"
#include "csg/color.h"
#include "qubicle_file_format.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

class Fabricator {
public:
   Fabricator(JSONNode const& data);
   ~Fabricator();

private:
};

END_RADIANT_VOXEL_NAMESPACE


#endif //  _RADIANT_VOXEL_QUBICLE_FABRICATOR_H
