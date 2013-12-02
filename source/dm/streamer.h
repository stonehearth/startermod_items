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
   void QueueAlloced();
   void QueueUpdate(tesseract::protocol::Update const& update);
   void QueueAllocs();
   void QueueAllocUpdates();
   void QueueDestroyed();
   void QueueUpdates();
   void OnModified(TraceBufferedRef t, ObjectRef o);
   void SetServerTick(int now);

private:
   Store const&            store_;
   protocol::SendQueue*    queue_;
   TracerBufferedPtr       tracer_;
   AllocTracePtr           alloc_trace_;
   std::unordered_map<ObjectId, TracePtr> dtor_traces_;
   int                     category_;
   std::map<ObjectId, ObjectRef>   alloced_;
   std::vector<ObjectId>           destroyed_;
   std::vector<std::pair<TraceBufferedRef, ObjectRef>>   updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STREAMER_H

