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
   dm::ObjectPtr GetObject(dm::Store const& store, std::string const& path) const;
   std::string GetPathToObject(dm::ObjectPtr obj) const;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_OBJECT_BROWSER_ENTITY_BROWSER_H
