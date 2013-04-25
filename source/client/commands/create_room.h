#ifndef _RADIANT_CLIENT_CREATE_ROOM_H
#define _RADIANT_CLIENT_CREATE_ROOM_H

#include "pending_command.h"
#include "client/selectors/voxel_range_selector.h"
#include "om/om.h"
#include "dm/dm.h"
#include "command.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class CreateRoom : public Command
{
public:
   CreateRoom(PendingCommandPtr cmd);
   ~CreateRoom();

   void operator()(void);

private:
   bool ResizeRoom(const csg::Point3& p0, const csg::Point3& p1);
   void SendCommand(const om::Selection& s);

private:
   std::shared_ptr<VoxelRangeSelector> selector_;
   om::EntityPtr                 room_;
   csg::Point3                   p0_;
   csg::Point3                   p1_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_CREATE_ROOM_H
