#include "radiant.h"
#include <boost/algorithm/string.hpp>
#include <Poco/Zip/ZipStream.h>
#include <Poco/StreamCopier.h>
#include "zip_module.h"
#include "exceptions.h"

using namespace ::radiant;
using namespace ::radiant::res;

ZipModule::ZipModule(std::string const& module_name, boost::filesystem::path const& zipfile_path) :
   module_name_(module_name),
   stream_(zipfile_path.string(), std::ios::binary),
   zip_(stream_)
{
}

ZipModule::~ZipModule()
{
}

bool ZipModule::CheckFilePath(std::vector<std::string> const& parts) const
{
   bool found = zip_.findHeader(GetQualifiedPath(boost::algorithm::join(parts, "/"))) != zip_.headerEnd();
   if (!found) {
      int dummy = 1;
   }
   return found;
}

std::shared_ptr<std::istream> ZipModule::OpenResource(std::string const& canonical_path) const
{
   auto i = zip_.findHeader(GetQualifiedPath(canonical_path));
   if (i == zip_.headerEnd()) {
      return nullptr;
   }

   Poco::Zip::ZipInputStream input(stream_, i->second);
   std::shared_ptr<std::stringstream> stream = std::make_shared<std::stringstream>();
   Poco::StreamCopier::copyStream(input, *stream);

   return stream;
}

std::string ZipModule::GetQualifiedPath(std::string const& resource_name) const
{
   return module_name_ + "/" + resource_name;
}
