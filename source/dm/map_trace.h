#ifndef _RADIANT_DM_MAP_TRACE_H
#define _RADIANT_DM_MAP_TRACE_H

#include "dm.h"
#include "trace_impl.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTrace : public TraceImpl<MapTrace<M>>
{
public:
   typedef typename M::Key             Key;
   typedef typename M::Value           Value;
   typedef std::vector<Key>            KeyList;
   typedef typename M::ContainerType   ChangeMap;

   typedef std::function<void(Key const& k)> RemovedCb;
   typedef std::function<void(Key const& k, Value const& v)> ChangedCb;
   typedef std::function<void(ChangeMap const& changed, KeyList const& removed)> UpdatedCb;

public:
   MapTrace(const char* reason, M const& m);
   std::shared_ptr<MapTrace> OnAdded(ChangedCb changed);
   std::shared_ptr<MapTrace> OnRemoved(RemovedCb removed);
   std::shared_ptr<MapTrace> OnUpdated(UpdatedCb updated);

protected:
   ObjectId GetObjectId() const;
   Store const& GetStore() const;
   void SignalRemoved(Key const& key);
   void SignalChanged(Key const& key, Value const& value) ;
   void SignalObjectState() override;
   void SignalUpdated(ChangeMap const& changed, KeyList const& removed);

protected:
   friend Store;
   virtual void NotifyRemoved(Key const& key) = 0;
   virtual void NotifyChanged(Key const& key, Value const& value) = 0;

private:
   M const&       map_;
   RemovedCb      removed_;
   ChangedCb      changed_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_H

