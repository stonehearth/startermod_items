#ifndef _RADIANT_HORDE3D_EXTENSIONS_SCREENSPACE_RECT_H
#define _RADIANT_HORDE3D_EXTENSIONS_SCREENSPACE_RECT_H

#include "namespace.h"
//#include "radiant.h"
//#include "Horde3D.h"

#include "screenspace.h"

using namespace ::Horde3D;
using namespace ::radiant::horde3d;

BEGIN_RADIANT_HORDE3D_NAMESPACE

class ScreenSpaceRect : ScreenSpaceShape
{
public:
   ScreenSpaceRect(H3DNode parent, int left, int right, int top, int bottom);
   ~ScreenSpaceRect();

protected:
};

END_RADIANT_HORDE3D_NAMESPACE

#endif
