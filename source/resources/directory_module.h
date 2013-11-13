#ifndef _RADIANT_RES_DIRECTORY_MODULE_H
#define _RADIANT_RES_DIRECTORY_MODULE_H

#include <boost/filesystem.hpp>
#include "imodule.h"

BEGIN_RADIANT_RES_NAMESPACE

class DirectoryModule : public IModule {
public:
   DirectoryModule(boost::filesystem::path const& path);
   virtual ~DirectoryModule();

   bool CheckFilePath(std::vector<std::string> const& parts) const override;
   std::shared_ptr<std::istream> OpenResource(std::string const& canonical_path) const override;

private:
   boost::filesystem::path    root_;
};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_DIRECTORY_MODULE_H
