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
   void OnAlloced(ObjectPtr object);
   void OnRegistered(Object const* obj);
   void OnDestroyed(ObjectId obj, bool dynamic);
   void OnModified(TraceBufferedRef t, Object const* obj);

   void QueueAllocated();
   void QueueUnsavedObjects();
   void QueueModifiedObjects();
   void QueueDestroyedObjects();

   void QueueUpdate(tesseract::protocol::Update const& update);
   void QueueAllocs();
   void QueueAllocUpdates();
   void QueueDestroyed();
   void QueueUpdates();
   void QueueUpdateInfo();

private:
   typedef std::vector<tesseract::protocol::Update> UpdateList;

private:
   Store const&            store_;
   protocol::SendQueue*    queue_;
   TracerBufferedPtr       tracer_;
   StoreTracePtr           store_trace_;
   std::unordered_map<ObjectId, TraceBufferedPtr> traces_;
   int                     category_;

   std::map<ObjectId, ObjectRef>          allocated_objects_;
   std::map<ObjectId, Object const*>      unsaved_objects_;
   std::unordered_map<ObjectId, UpdateList>  object_updates_;
   std::map<ObjectId, bool>               destroyed_objects_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STREAMER_H

