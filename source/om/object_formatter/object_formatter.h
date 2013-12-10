#ifndef _RADIANT_OM_OBJECT_FORMATTER_H
#define _RADIANT_OM_OBJECT_FORMATTER_H

#include "libjson.h"
#include "om/namespace.h"
#include "dm/dm.h"

BEGIN_RADIANT_OM_NAMESPACE

class ObjectFormatter
{
public:
   ObjectFormatter();
   JSONNode ObjectToJson(dm::ObjectPtr obj) const;
   bool IsPathInStore(dm::Store const& store, std::string const& path) const;
   dm::ObjectPtr GetObject(dm::Store const& store, std::string const& path) const;
   template <typename T> std::shared_ptr<T> GetObject(dm::Store const& store, std::string const& path) const {
      dm::ObjectPtr obj = GetObject(store, path);
      return std::dynamic_pointer_cast<T>(obj);
   }
   std::string GetPathToObject(dm::ObjectPtr obj) const;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_OBJECT_BROWSER_ENTITY_BROWSER_H
