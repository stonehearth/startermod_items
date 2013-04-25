#ifndef _RADIANT_SIMULATION_CREATE_ROOM_CMD_H
#define _RADIANT_SIMULATION_CREATE_ROOM_CMD_H

#include "om/om.h"
#include "tesseract.pb.h"
#include "namespace.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class CreateRoomCmd
{
public:
   std::string operator()(const tesseract::protocol::DoAction& msg) const;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_CREATE_ROOM_CMD_H
