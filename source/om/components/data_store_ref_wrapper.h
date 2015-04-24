#ifndef _RADIANT_OM_DATA_STORE_REF_WRAPPER_H
#define _RADIANT_OM_DATA_STORE_REF_WRAPPER_H

#include "om/om.h"

BEGIN_RADIANT_OM_NAMESPACE
   
class DataStoreRefWrapper
{
public:
   DataStoreRefWrapper(om::DataStoreRef ds) : _ds(ds) { }

   om::DataStorePtr lock() { return _ds.lock(); }

   bool operator==(DataStoreRefWrapper const& rhs) const {
      return _ds.lock() == rhs._ds.lock();
   }
private:
   om::DataStoreRef _ds;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_DATA_STORE_REF_WRAPPER_H
