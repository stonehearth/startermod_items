#ifndef _RADIANT_LIB_JSON_CORE_CORE_JSON_H
#define _RADIANT_LIB_JSON_CORE_CORE_JSON_H

#include "core/input.h"
#include "lib/json/node.h"

BEGIN_RADIANT_JSON_NAMESPACE
  
template <class T> Node encode(std::shared_ptr<T> const &obj)
{
   if (obj) {
      return encode(*obj);
   }
   return Node();
}

template <class T> Node encode(std::weak_ptr<T> const &o)
{
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      return encode(*obj);
   }
   return Node();
}

DECLARE_JSON_CODEC_TEMPLATES(MouseInput)
DECLARE_JSON_CODEC_TEMPLATES(KeyboardInput)
DECLARE_JSON_CODEC_TEMPLATES(RawInput)
DECLARE_JSON_CODEC_TEMPLATES(Input)

END_RADIANT_JSON_NAMESPACE

#endif // _RADIANT_LIB_JSON_CORE_CORE_JSON_H
