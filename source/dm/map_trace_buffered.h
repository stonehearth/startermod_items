#ifndef _RADIANT_DM_MAP_TRACE_BUFFERED_H
#define _RADIANT_DM_MAP_TRACE_BUFFERED_H

#include "dm.h"
#include "trace_buffered.h"
#include "map_trace.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTraceBuffered : public core::ObjectCounter<MapTraceBuffered<M>>, 
                         public TraceBuffered,
                         public MapTrace<M>
                         
{
public:
   MapTraceBuffered(const char* reason, M const& m);
   virtual ~MapTraceBuffered();

   void Flush();
   bool SaveObjectDelta(SerializationType r, Protocol::Value* value) override;

private:
   void ClearCachedState() override;
   void NotifyRemoved(Key const& key) override;
   void NotifyChanged(Key const& key, Value const& value) override;

private:
   bool                 firing_;
   ChangeMap            changed_, pending_changed_;
   KeyList              removed_, pending_removed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_BUFFERED_H

