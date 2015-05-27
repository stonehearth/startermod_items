#include <sstream>
#include "radiant.h"
#include "sysinfo.h"

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

HMODULE psapiDll;

// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms683219%28v=vs.85%29.aspx for why
// we have to do this to avoid crashing on Vista. =( - tony
typedef BOOL (WINAPI *GetProcessMemoryInfoFn)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
static GetProcessMemoryInfoFn GetProcessMemoryInfof;

#define SI_LOG(level)              LOG(sysinfo, level)

struct LoadGetProcessMemoryInfo {
   LoadGetProcessMemoryInfo() {
      psapiDll = LoadLibrary("psapi.dll");
      if (psapiDll) {
         GetProcessMemoryInfof = (GetProcessMemoryInfoFn)GetProcAddress(psapiDll, "GetProcessMemoryInfo");
      }
   }
};
static LoadGetProcessMemoryInfo loadGetProcessMemoryInfo;

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif
#endif

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

void SysInfo::GetVirtualAddressSpaceUsage(platform::SysInfo::ByteCount& total, platform::SysInfo::ByteCount& avail)
{
   MEMORYSTATUSEX mse = { 0 };
   mse.dwLength = sizeof(MEMORYSTATUSEX);
   if (GlobalMemoryStatusEx(&mse)) {
      // From MSDN...
      //
      // ullTotalVirtual - The size of the user-mode portion of the virtual address
      // space of the calling process, in bytes. This value depends on the type of
      // process, the type of processor, and the configuration of the operating
      // system. For example, this value is approximately 2 GB for most 32-bit processes
      // on an x86 processor and approximately 3 GB for 32-bit processes that are
      // large address aware running on a system with 4-gigabyte tuning enabled.
      //
      // ullAvailVirtual - The amount of unreserved and uncommitted memory currently
      // in the user-mode portion of the virtual address space of the calling
      // process, in bytes.
      //
      total = mse.ullTotalVirtual;
      avail = mse.ullAvailVirtual;
   } else {
      total = 0;
      avail = 0;
   }
}

platform::SysInfo::ByteCount SysInfo::GetCurrentMemoryUsage()
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */

   if (GetProcessMemoryInfof) {
      PROCESS_MEMORY_COUNTERS info = { 0 };
      if (GetProcessMemoryInfof(GetCurrentProcess( ), &info, sizeof(info))) {
         return (platform::SysInfo::ByteCount)info.PagefileUsage;
      }
   }
   return (platform::SysInfo::ByteCount)0;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
		(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;		/* Can't access? */
	return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
		return (size_t)0L;		/* Can't open? */
	if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
	{
		fclose( fp );
		return (size_t)0L;		/* Can't read? */
	}
	fclose( fp );
	return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;			/* Unsupported. */
#endif
}

platform::SysInfo::ByteCount SysInfo::GetTotalSystemMemory()
{
   unsigned long long result = 0;
   #ifdef _WIN32
   MEMORYSTATUSEX status;
   status.dwLength = sizeof(status);
   GlobalMemoryStatusEx(&status);
   result = status.ullTotalPhys;
   #endif

   return result;
}

std::string SysInfo::GetOSName()
{
   std::string value(256, 0);
   DWORD size = (DWORD)value.size();
   if (RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName", RRF_RT_REG_SZ, nullptr, &value[0], &size) == ERROR_SUCCESS) {
      // Make sure we don't include the NULL (this string is double null terminated!)
      size = (DWORD)strlen(value.c_str());
      value.resize(size);
      return value;
   }
   return "";
}


void SysInfo::LogMemoryStatistics(const char* reason, uint level)
{
   platform::SysInfo::ByteCount va_total, va_avail;
   platform::SysInfo::GetVirtualAddressSpaceUsage(va_total, va_avail);

   static auto logMemoryUsage = [level](const char* str, platform::SysInfo::ByteCount memory) {
      static const char* suffix[] = {
         "B",
         "KB",
         "MB",
         "GB",
         "TB",
         "PB", // HA!
      };
      int i = 0;
      double friendly = static_cast<double>(memory);
      while (friendly > 1024 && i < ARRAY_SIZE(suffix) - 1) {
         friendly /= 1024;
         i++;
      }
      SI_LOG(level) << " " << str << std::setprecision(3) << std::fixed << friendly << " " << suffix[i] << " (" << memory << " bytes)";
   };
   SI_LOG(level) << "Memory Stats: " << reason;
   logMemoryUsage("Total System Memory:     ", platform::SysInfo::GetTotalSystemMemory());
   logMemoryUsage("Current Memory Usage:    ", platform::SysInfo::GetCurrentMemoryUsage());
   logMemoryUsage("Total Address Space:     ", va_total);
   logMemoryUsage("Available Address Space: ", va_avail);
   logMemoryUsage("Used Address Space:      ", va_total - va_avail);
}
