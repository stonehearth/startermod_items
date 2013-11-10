#include "radiant.h"
#include <boost/algorithm/string.hpp>
#include <Poco/Zip/ZipStream.h>
#include <Poco/StreamCopier.h>
#include "zip_module.h"
#include "exceptions.h"

using namespace ::radiant;
using namespace ::radiant::res;

namespace fs = ::boost::filesystem;

ZipModule::ZipModule(fs::path const& path) :
   stream_(path.string(), std::ios::binary),
   zip_(stream_)
{
}

ZipModule::~ZipModule()
{
}

bool ZipModule::CheckFilePath(std::vector<std::string> const& parts) const
{
   return zip_.findHeader(boost::algorithm::join(parts, "/")) != zip_.headerEnd();
}

#include "radiant_file.h"

std::shared_ptr<std::istream> ZipModule::OpenResource(std::string const& canonical_path) const
{
   auto i = zip_.findHeader(canonical_path);
   if (i == zip_.headerEnd()) {
      return nullptr;
   }

   Poco::Zip::ZipInputStream input(stream_, i->second);
   std::shared_ptr<std::stringstream> stream = std::make_shared<std::stringstream>();
   Poco::StreamCopier::copyStream(input, *stream);

   return stream;
}