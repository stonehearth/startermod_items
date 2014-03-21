#include <unordered_map>
#include "radiant.h"
#include "dispatcher.h"
#include "dispatch_table.h"

using namespace radiant;
using namespace radiant::typeinfo;

static void RegisterType(TypeId id, DispatchTable const& dispatch_table);