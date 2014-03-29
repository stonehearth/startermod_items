#ifndef _RADIANT_DM_RECEIVER_H
#define _RADIANT_DM_RECEIVER_H

#include "dm.h"
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"
#include "protocol.h"
#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

class Receiver
{
public:
   //using ::radiant::tesseract::protocol;
   Receiver(Store& store);

   void ProcessAlloc(tesseract::protocol::AllocObjects const& update);
   void ProcessUpdate(tesseract::protocol::UpdateObject const& update);
   void ProcessRemove(tesseract::protocol::RemoveObjects const& update);

   void Shutdown();

private:
   typedef std::unordered_map<ObjectId, ObjectPtr> ObjectMap;

private:
   Store&      store_;
   ObjectMap   objects_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECEIVER_H

