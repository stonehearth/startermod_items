#ifndef _RADIANT_DM_STREAMER_H
#define _RADIANT_DM_STREAMER_H

#include "dm.h"
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"
#include "protocol.h"
#include "map_loader.h"
#include "record_loader.h"
#include "set_loader.h"
#include "boxed_loader.h"
#include "array_loader.h"

BEGIN_RADIANT_DM_NAMESPACE

class Streamer
{
public:
   //using ::radiant::tesseract::protocol;
   Streamer(Store const& store, int category, protocol::SendQueue* queue)
   {
      alloc_trace_ = store.TraceAlloc()
         ->
   }


   void Flush();

   // xxx: THIS IS THE ONE!
   
   template <typename C>
   std::shared_ptr<MapTrace<C>> TraceMapChanges(const char* reason, Store const& store, C const& map)
   {
      ObjectId id = map.GetObjectId();
      SaveObjectState(map);

      auto trace = TracerBuffered::TraceMapChanges(reason, store, map);
      trace->OnUpdated([this, id](MapTrace<C>::ChangeMap const& changed, MapTrace<C>::KeyList const& removed) {
         Protocol::Object* msg = StartUpdate(id);
         if (msg) {
            EncodeDelta(changed, removed, msg);
            FinishUpdate(msg);
         }
      });
      return trace;
   }

   template <typename C>
   std::shared_ptr<MapTrace<C>> TraceMapChanges(const char* reason, Store const& store, C const& map)
   {
      ObjectId id = map.GetObjectId();
      SaveObjectState(map);

      auto trace = TracerBuffered::TraceMapChanges(reason, store, map);
      trace->OnUpdated([this, id](MapTrace<C>::ChangeMap const& changed, MapTrace<C>::KeyList const& removed) {
         Protocol::Object* msg = StartUpdate(id);
         if (msg) {
            EncodeDelta(changed, removed, msg);
            FinishUpdate(msg);
         }
      });
      return trace;
   }

   template <typename C>
   std::shared_ptr<SetTrace<C>> TraceSetChanges(const char* reason, Store const& store, C const& set)
   {
      ObjectId id = map.GetObjectId();
      SaveObjectState(set);

      auto trace = TracerBuffered::TraceSetChanges(reason, store, set);
      trace->OnUpdated([this, id](SetTrace<C>::ValueList const& added, SetTrace<C>::ValueList const& removed) {
         Protocol::Object* msg = StartUpdate(id);
         if (msg) {
            EncodeDelta(added, removed, msg);
            FinishUpdate(msg);
         }
      });
      return trace;
   }

   template <typename C>
   std::shared_ptr<ArrayTrace<C>> TraceArrayChanges(const char* reason, Store const& store, C const& arr)
   {
      ObjectId id = arr.GetObjectId();
      SaveObjectState(arr);

      auto trace = TracerBuffered::TraceArrayChanges(reason, store, arr);
      trace->OnUpdated([this, id](ArraTrace<C>::ChangeMap const& changed) {
         Protocol::Object* msg = StartUpdate(id);
         if (msg) {
            EncodeDelta(changed, msg);
            FinishUpdate(msg);
         }
      });
      return trace;
   }

   template <typename C>
   std::shared_ptr<BoxedTrace<C>> TraceBoxedChanges(const char* reason, Store const& store, C const& boxed)
   {
      ObjectId id = boxed.GetObjectId();
      SaveObjectState(boxed);

      auto trace = TracerBuffered::TraceBoxedChanges(reason, store, boxed);
      trace->OnChanged([this, id](BoxedTrace<C>::Value const& value) {
         Protocol::Object* msg = StartUpdate(id);
         if (msg) {
            EncodeDelta(value, msg);
            FinishUpdate(msg);
         }
      });
      return trace;
   }

   template <typename C>
   std::shared_ptr<RecordTrace<C>> TraceRecordChanges(const char* reason, Store const& store, C const& record)
   {
      SaveObjectState(record);
      // Nothing to do for records aside from sending the initial layout
      return TracerBuffered::TraceRecordChanges(reason, store, record);
   }

private:
   template <typename T> void SaveObjectState(T const& obj)
   {
      Protocol::Object* msg = StartUpdate(obj.GetObjectId());
      if (msg) {
         SaveObject(obj, msg->mutable_value());
         FinishUpdate(msg);
      }
   }

private:
   Protocol::Object* StartUpdate(ObjectId id);
   void FinishUpdate(::Protocol::Object* msg);

private:
   Store const&                  store_;
   TracerBufferedPtr             tracer_;
   tesseract::protocol::Update   update_;
   protocol::SendQueue*          queue_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STREAMER_H

