#include "radiant.h"
#include "lib/json/node.h"
#include "lib/json/core_json.h"
#include "core/static_string.h"

using namespace radiant;
using namespace radiant::json;

DEFINE_INVALID_JSON_CONVERSION(MouseInput)
DEFINE_INVALID_JSON_CONVERSION(KeyboardInput)
DEFINE_INVALID_JSON_CONVERSION(RawInput)
DEFINE_INVALID_JSON_CONVERSION(Input)


template <> Node json::encode(core::StaticString const& n) {
   return JSONNode("", n);
}
