#include <sstream>
#include <shlobj.h>
#include "radiant.h"
#include "system.h"
#include "radiant_exceptions.h"
#include "radiant_macros.h"
#include "build_number.h"
#include "poco/UUIDGenerator.h"
#include <io.h>

using namespace ::radiant;
using namespace ::radiant::core;

DEFINE_SINGLETON(System)

System::System()
{
   boost::filesystem::path const run_directory = boost::filesystem::canonical(".");

   if (IsWritableDirectory(run_directory.string())) {
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

bool System::IsWritableDirectory(std::string const& directory) const
{
   std::string out_text;
   std::string in_text = "DeleteMe";
   std::string test_filename = Poco::UUIDGenerator::defaultGenerator().create().toString() + ".txt";
   std::string full_filename = directory + "/" + test_filename;

   try {
      std::ofstream ostream(full_filename);
      ostream << in_text;
      ostream.close();

      std::ifstream istream(full_filename);
      istream >> out_text;
      istream.close();

      boost::filesystem::remove(full_filename);

      return out_text == in_text;
   } catch (...) {
      return false;
   }
}

boost::filesystem::path System::GetTempDirectory() const
{
   return temp_directory_;
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

bool System::IsProcess64Bit()
{
#if defined(_M_X64) || defined(__amd64__)
   return true;
#endif
   return false;
}

bool System::IsPlatform64Bit()
{
#if defined(_M_X64) || defined(__amd64__)
   return true;
#else
   BOOL bIsWow64 = FALSE;

   // IsWow64Process is not available on all supported versions of Windows.
   // Use GetModuleHandle to get a handle to the DLL that contains the function
   // and GetProcAddress to get a pointer to the function if available.
   typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

   LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
   if (!fnIsWow64Process) {
      return false;
   }
   if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
      return false;
   }
   return !!bIsWow64;
#endif
}

