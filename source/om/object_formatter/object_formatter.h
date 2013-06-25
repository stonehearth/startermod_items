#ifndef _RADIANT_OM_OBJECT_FORMATTER_H
#define _RADIANT_OM_OBJECT_FORMATTER_H

#include "libjson.h"
#include "om/namespace.h"
#include "dm/dm.h"

BEGIN_RADIANT_OM_NAMESPACE

class ObjectFormatter
{
public:
   ObjectFormatter(std::string const& root);
   JSONNode ObjectToJson(dm::ObjectPtr obj) const;
   std::string GetPathToObject(dm::ObjectPtr obj) const;

private:
   std::string    root_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_OBJECT_BROWSER_ENTITY_BROWSER_H
