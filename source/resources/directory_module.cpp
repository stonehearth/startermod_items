#include "radiant.h"
#include <boost/algorithm/string.hpp>
#include "directory_module.h"
#include "exceptions.h"

using namespace ::radiant;
using namespace ::radiant::res;

namespace fs = ::boost::filesystem;

DirectoryModule::DirectoryModule(fs::path const& path) :
   root_(path)
{
}

DirectoryModule::~DirectoryModule()
{
}

bool DirectoryModule::CheckFilePath(std::vector<std::string> const& parts) const
{
   fs::path filepath = root_;
   for (std::string part : parts) {
      filepath /= part;
   }

   return fs::is_regular_file(filepath);
}

std::shared_ptr<std::istream> DirectoryModule::OpenResource(std::string const& canonical_path) const
{
   std::string path = (root_ / canonical_path).string();
   std::shared_ptr<std::ifstream> in = std::make_shared<std::ifstream>(path, std::ios::in | std::ios::binary);
   if (!in->good()) {
      return nullptr;
   }
   return std::static_pointer_cast<std::istream>(in);
}