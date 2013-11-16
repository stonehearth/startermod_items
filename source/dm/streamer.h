#ifndef _RADIANT_DM_STREAMER_H
#define _RADIANT_DM_STREAMER_H

#include "dm.h"
#include "store.pb.h"
#include "tesseract.pb.h"
#include "protocol.h"
#include "tracer_buffered.h"
#include "map_loader.h"

BEGIN_RADIANT_DM_NAMESPACE

class Streamer : public TracerBuffered
{
public:
   //using ::radiant::tesseract::protocol;

   Streamer(dm::Store& store);
   TracerType GetType() const override;

   // xxx: this should move to the constructor once the arbiter is smart
   // enough to let the simulation handle queue lifetime.
   void Flush(protocol::SendQueue* queue);

   template <typename C>
   std::shared_ptr<MapTrace<C>> TraceMapChanges(const char* reason, C const& map)
   {
      auto trace = TracerBuffered::TraceMapChanges(reason, map);

      ObjectId id = map.GetObjectId();
      trace->OnUpdate([this, id](std::vector<C::Entry> const& changed, std::vector<C::Key> const& removed) {
         Protocol::Object* msg = StartUpdate(obj);
         if (msg) {
            Save(changed, removed, msg);
            FinishUpdate(msg);
         }
      });
      return trace;
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

