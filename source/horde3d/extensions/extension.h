#ifndef _RADIANT_HORDE3D_EXTENSIONS_EXTENSION_H
#define _RADIANT_HORDE3D_EXTENSIONS_EXTENSION_H

#include "../Source/Horde3DEngine/egPrerequisites.h"
#include "../Source/Horde3DEngine/egExtensions.h"
#include "namespace.h"

static const int SNT_DebugShapesNode   = 1000;
static const int SNT_StockpileNode     = 1001;
static const int SNT_DecalNode         = 1002;
static const int SNT_ToastNode         = 1003;
static const int SNT_RegionNode        = 1005;
static const int SNT_CubemitterNode    = 1006;
static const int RT_CubemitterResource = 1007;

BEGIN_RADIANT_HORDE3D_NAMESPACE

class Extension : public Horde3D::IExtension
{
public:
   Extension();
   ~Extension();

public:
	virtual const char *getName();
	virtual bool init();
	virtual void release();
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
