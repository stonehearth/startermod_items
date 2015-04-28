#ifndef _RADIANT_DM_STREAMER_H
#define _RADIANT_DM_STREAMER_H

#include "dm.h"
#include "protocols/forward_defines.h"
#include "protocol.h"
#include <unordered_map>

BEGIN_RADIANT_DM_NAMESPACE

class Streamer
{
public:
   Streamer(Store& store, int category, protocol::SendQueue* queue);
   ~Streamer();

   void Flush();

public: // Streamer Object Interafce 
   template <typename T> void TraceObject(T const* obj);
   void OnAlloced(ObjectPtr const& object);
   void OnModified(Object const* obj);
   void OnDestroyed(ObjectId obj, bool dynamic);

private:
   void SaveDeltaState(TraceBufferedRef t, Object const* obj);

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
   void AddUnsavedObject(Object const* obj);

   typedef std::vector<tesseract::protocol::Update> UpdateList;

private:
   Store&                  store_;
   protocol::SendQueue*    queue_;
   TracerBufferedPtr       tracer_;
   std::unordered_map<ObjectId, TraceBufferedPtr> traces_;
   int                     category_;

   std::map<ObjectId, ObjectRef>          allocated_objects_;
   std::map<ObjectId, Object const*>      unsaved_objects_;
   std::unordered_map<ObjectId, UpdateList>  object_updates_;
   std::map<ObjectId, bool>               destroyed_objects_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STREAMER_H

