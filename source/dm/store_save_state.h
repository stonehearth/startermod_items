#ifndef _RADIANT_DM_STORE_SAVE_STATE_H
#define _RADIANT_DM_STORE_SAVE_STATE_H

#include <unordered_map>
#include <google/protobuf/io/coded_stream.h>
#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

std::ostream& operator<<(std::ostream& os, StoreSaveState const& in);

class StoreSaveState
{
public:
   StoreSaveState();

   void Save(Store& store, ObjectId id);   
   void Load(Store& store);

private:
   friend std::ostream& operator<<(std::ostream& os, StoreSaveState const& in);
   NO_COPY_CONSTRUCTOR(StoreSaveState);

   ObjectId                _id;
   std::string             _saveState;
};

END_RADIANT_DM_NAMESPACE


#endif // _RADIANT_DM_STORE_SAVE_STATE_H
