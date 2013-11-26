#include <shlobj.h>
#include "radiant.h"
#include "system.h"
#include "radiant_exceptions.h"
#include "radiant_macros.h"
#include "build_number.h"
#include <sstream>

using namespace ::radiant;
using namespace ::radiant::core;

DEFINE_SINGLETON(System)

System::System()
{
   // "ProgramFiles(x86)" for 32-bit programs, "ProgramFiles" for 64-bit programs
   std::string const program_files_var_name = "ProgramFiles(x86)";
   boost::filesystem::path const program_files_directory(getenv(program_files_var_name.c_str()));
   boost::filesystem::path const run_directory = boost::filesystem::canonical(".");

   if (IsAncestorDirectory(program_files_directory, run_directory)) {
      temp_directory_ = run_directory;
   } else {
      temp_directory_ = DefaultTempDirectory(PRODUCT_IDENTIFIER);

      if (!boost::filesystem::is_directory(temp_directory_)) {
         try {
            boost::filesystem::create_directory(temp_directory_);
         } catch (std::exception const& e) {
            throw core::Exception(BUILD_STRING("Cannot create temp directory " << temp_directory_ << ":\n\n" << e.what()));
         }
      }
   }
}

boost::filesystem::path System::GetTempDirectory() const
{
   return temp_directory_;
}

bool System::IsAncestorDirectory(boost::filesystem::path const& ancestor, boost::filesystem::path const& directory) const
{
   std::string const ancestor_str = boost::filesystem::canonical(ancestor).string();
   std::string const directory_str = boost::filesystem::canonical(directory).string();
   size_t const index = directory_str.find(ancestor_str);
   return index == 0;
}

boost::filesystem::path System::DefaultTempDirectory(std::string const& application_name) const
{
   TCHAR path[MAX_PATH];
   HRESULT const result = SHGetFolderPath(nullptr,               // reserved
                                          CSIDL_LOCAL_APPDATA,   // folder type
                                          nullptr,               // access token
                                          0,                     // flags
                                          path);                 // returned path
   if (result != S_OK) {
      throw std::invalid_argument("Could not retrieve path to local application data");
   }
   return boost::filesystem::path(path) / application_name;
}
