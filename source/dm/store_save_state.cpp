#include "radiant.h"
#include "radiant_stdutil.h"
#include "store_save_state.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

std::ostream& dm::operator<<(std::ostream& os, StoreSaveState const& s)
{
   return (os << "[SaveState " << s._saveState.size() << " bytes]");
}

StoreSaveState::StoreSaveState()
{
}

void StoreSaveState::AddObject(ObjectId id)
{
   stdutil::UniqueInsert(_toSave, id);
}

void StoreSaveState::Save(Store& store)
{
   std::string error;
   _saveState = store.SaveObjects(_toSave, error);
}

ObjectMap StoreSaveState::Load(Store& store)
{
   std::string error;
   ObjectMap objects;

   if (!store.LoadObjects(_saveState, objects, error)) {
      throw std::logic_error(BUILD_STRING("error while loading: " << error));
   }
   return objects;
}
