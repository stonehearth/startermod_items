#ifndef _RADIANT_CLIENT_RENDERER_PCH_H
#define _RADIANT_CLIENT_RENDERER_PCH_H

#include "radiant.h"
#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/property_tree/ptree.hpp>

#include "radiant_stdutil.h"
#include "lib/lua/bind.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"
#include "Horde3DRadiant.h"
#include "render_entity.h"

#include "om/all_objects.h"
#include "om/all_components.h"

#endif // _RADIANT_CLIENT_RENDERER_PCH_H