#ifndef _RADIANT_DM_MAP_TRACE_H
#define _RADIANT_DM_MAP_TRACE_H

#include "dm.h"
#include "trace.h"

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
   MapTrace(const char* reason, Object const& o, Store const& store) :
      TraceImpl(reason, o, store)
   {
   }

   std::shared_ptr<MapTrace> OnChanged(ChangedCb changed)
   {
      changed_ = changed;
      return shared_from_this();
   }

   std::shared_ptr<MapTrace>  OnRemoved(RemovedCb removed)
   {
      removed_ = removed;
      return shared_from_this();
   }

   std::shared_ptr<MapTrace>  OnUpdated(UpdatedCb updated)
   {
      updated_ = updated;
      return shared_from_this();
   }

   std::shared_ptr<MapTrace> PushObjectState()
   {
      GetStore().PushMapState(*this, GetObjectId());
      return shared_from_this();
   }

protected:
   void SignalRemoved(Key const& key)
   {
      if (removed_) {
         removed_(key);
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalChanged(Key const& key, Value const& value) 
   {
      if (changed) {
         changed_(key, value)
      } else if (updated_) {
         NOT_YET_IMPLEMENTED();
      }
   }

   void SignalUpdated(ChangeMap const& changed, KeyList const& removed)
   {
      if (updated_) {
         updated_(changed, removed);
      } else {
         if (changed_) {
            for (const auto& entry : changed) {
               changed_(entry.first, entry.second);
            }
         }

         if (removed_) {
            for (const auto& key : removed) {
               removed_(key);
            }
         }
      }
   }

private:
   friend Store;
   virtual void NotifyRemoved(Key const& key);
   virtual void NotifyChanged(Key const& key, Value const& value);
   virtual void NotifyObjectState(typename M::ContainerType const& contents)
   {
      SignalUpdated(contents, KeyList());
   }

private:
   RemovedCb      removed_;
   ChangedCb      changed_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_H

