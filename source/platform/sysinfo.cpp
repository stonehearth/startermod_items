#include <sstream>
#include "radiant.h"
#include "sysinfo.h"

using namespace ::radiant;
using namespace ::radiant::platform;


std::string SysInfo::GetOSVersion()
{
   DWORD size = GetFileVersionInfoSize("kernel32.dll", nullptr);
   if (size) {
      std::vector<uchar> data(size, 0);
      LPVOID block = &data[0];

      if (GetFileVersionInfo("kernel32.dll", 0, size, block)) {
         struct LANGANDCODEPAGE {
           WORD wLanguage;
           WORD wCodePage;
         } *translation = nullptr;
         UINT translation_len = 0;

         VerQueryValue(block, "\\VarFileInfo\\Translation", (LPVOID*)&translation, &translation_len);
         if (translation && translation_len >= sizeof(WORD) * 2) {
            TCHAR key[256];
            sprintf(key, "\\StringFileInfo\\%04x%04x\\ProductVersion", translation->wLanguage, translation->wCodePage);

            TCHAR* version = nullptr;
            UINT version_len = 0;
            VerQueryValue(block, key, (LPVOID*)&version, &version_len);
            if (version && version_len) {
               return std::string(version);
            }
         }
      }
   }
   return "";
}

std::string SysInfo::GetOSName()
{
   std::string value(256, 0);
   DWORD size = value.size();
   if (RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName", RRF_RT_REG_SZ, nullptr, &value[0], &size) == ERROR_SUCCESS) {
      // Make sure we don't include the NULL (this string is double null terminated!)
      size = strlen(value.c_str());
      value.resize(size);
      return value;
   }
   return "";
}
