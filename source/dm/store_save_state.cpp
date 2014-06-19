#include "radiant.h"
#include "radiant_stdutil.h"
#include "store_save_state.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

std::ostream& dm::operator<<(std::ostream& os, StoreSaveState const& s)
{
   return (os << "[SaveState id:" << s._id << " size:" << s._saveState.size() << " bytes]");
}

StoreSaveState::StoreSaveState() :
   _id(0)
{
}

void StoreSaveState::Save(Store& store, ObjectId id)
{
   std::string error;
   _id = id;
   _saveState = store.SaveObject(id, error);
}

void StoreSaveState::Load(Store& store)
{
   std::string error;
   if (!store.LoadObject(_saveState, error)) {
      throw std::logic_error(BUILD_STRING("error while loading: " << error));
   }
}
