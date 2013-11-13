#ifndef _RADIANT_RES_IMODULE_H
#define _RADIANT_RES_IMODULE_H

#include "namespace.h"

BEGIN_RADIANT_RES_NAMESPACE

class IModule {
public:
   virtual ~IModule() { }

   virtual bool CheckFilePath(std::vector<std::string> const& parts) const = 0;
   virtual std::shared_ptr<std::istream> OpenResource(std::string const& canonical_path) const = 0;

};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_IMODULE_H
