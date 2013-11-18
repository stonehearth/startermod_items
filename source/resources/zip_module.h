#ifndef _RADIANT_RES_ZIP_MODULE_H
#define _RADIANT_RES_ZIP_MODULE_H

#include <boost/filesystem.hpp>
#include <Poco/Zip/ZipArchive.h>
#include "imodule.h"

BEGIN_RADIANT_RES_NAMESPACE

class ZipModule : public IModule {
public:
   ZipModule(std::string const& module_name, boost::filesystem::path const& path);
   virtual ~ZipModule();

   bool CheckFilePath(std::vector<std::string> const& parts) const override;
   std::shared_ptr<std::istream> OpenResource(std::string const& canonical_path) const override;

private:
   std::string GetQualifiedPath(std::string const& resource_name) const;

   mutable std::ifstream      stream_;
   Poco::Zip::ZipArchive      zip_;
   std::string const          module_name_;
};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_ZIP_MODULE_H
