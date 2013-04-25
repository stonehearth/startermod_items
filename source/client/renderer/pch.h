#ifndef _RADIANT_CLIENT_RENDERER_PCH_H
#define _RADIANT_CLIENT_RENDERER_PCH_H

#include "radiant.h"
#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif

#include "radiant_stdutil.h"
#include "radiant_luabind.h"
#include "math3d.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"
#include "render_entity.h"
#include <boost/property_tree/ptree.hpp>

#define DEFINE_ALL_OBJECTS
#include "om/all_objects.h"
#include "om/all_object_types.h"

#endif // _RADIANT_CLIENT_RENDERER_PCH_H