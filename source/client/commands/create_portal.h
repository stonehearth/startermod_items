#ifndef _RADIANT_CLIENT_CREATE_PORTAL_H
#define _RADIANT_CLIENT_CREATE_PORTAL_H

#include "pending_command.h"
#include "client/xz_region_selector.h"
#include "om/om.h"
#include "dm/dm.h"
#include "command.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class CreatePortal : public std::enable_shared_from_this<CreatePortal>,
                     public Command
{
public:
   CreatePortal(PendingCommandPtr cmd);
   ~CreatePortal();

   void operator()(void);

private:
   bool ResizePortal(const csg::Point3& p0, const csg::Point3& p1);
   void SendCommand();
   void OnMouseEvent(const MouseEvent &evt, bool &handled, bool &uninstall);
   void BeginShadowing(om::EntityPtr wall);
   void EndShadowing();
   void MovePortal(const csg::Point3& location);
   void FindShadowWall(int x, int y);
   bool UpdatePortalPosition(int x, int y);

private:
   om::EntityRef        remoteWall_;
   om::EntityPtr        shadowWall_;
   math3d::transform    shadowWallTransform_;
   om::EntityPtr        portal_;
   math3d::ipoint3      portalPosition_;
   csg::Rect2           portalBounds_;
   int                  inputHandlerId_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_CREATE_PORTAL_H
