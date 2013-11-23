#ifndef _RADIANT_DM_STREAMER_H
#define _RADIANT_DM_STREAMER_H

#include "dm.h"
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"
#include "protocol.h"
#include <unordered_map>

BEGIN_RADIANT_DM_NAMESPACE

class Streamer
{
public:
   Streamer(Store& store, int category, protocol::SendQueue* queue);

   void Flush();

private:
   void QueueUpdate(tesseract::protocol::Update const& update);
   void OnAlloced(std::vector<ObjectPtr> const& objects);
   void OnModified(TraceBufferedRef t, ObjectRef o);
   void OnDestroyed(ObjectId id);
   void SetServerTick(int now);

private:
   protocol::SendQueue*    queue_;
   AllocTracePtr           alloc_trace_;
   TracerBufferedPtr       tracer_;
   std::unordered_map<ObjectId, TracePtr> dtor_traces_;
   int                     category_;
   tesseract::protocol::Update   destroyed_update_;
   tesseract::protocol::RemoveObjects *destroyed_msg_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STREAMER_H

