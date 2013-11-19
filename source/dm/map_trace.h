#ifndef _RADIANT_DM_MAP_TRACE_H
#define _RADIANT_DM_MAP_TRACE_H

#include "dm.h"
#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename M>
class MapTrace : public Trace
{
public:
   typedef typename M::Key       Key;
   typedef typename M::Value     Value;
   typedef std::vector<Key>      KeyList;
   typedef std::unordered_map<Key, Value> ChangeMap;

   typedef std::function<void(Key const& k)> RemovedCb;
   typedef std::function<void(Key const& k, Value const& v)> ChangedCb;
   typedef std::function<void(ChangeMap const& changed, KeyList const& removed)> UpdatedCb;

public:
   void OnChanged(ChangedCb changed)
   {
      changed_ = changed;
   }

   void OnRemoved(RemovedCb removed)
   {
      removed_ = removed;
   }

   void OnUpdated(UpdatedCb updated)
   {
      updated_ = updated;
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
         update_(changed, removed);
      } else {
         for (const auto& entry : changed) {
            SignalChanged(entry.first, entry.second);
         }

         for (const auto& key : removed) {
            SignalRemoved(key);
         }
      }
   }

private:
   friend Store;
   virtual void OnRemoved(Key const& key);
   virtual void OnChanged(Key const& key, Value const& value);

private:
   RemovedCb      removed_;
   ChangedCb      changed_;
   UpdatedCb      updated_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_TRACE_H

