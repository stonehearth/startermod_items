#ifndef _RADIANT_PLATFORM_SYSINFO_H
#define _RADIANT_PLATFORM_SYSINFO_H

#include "namespace.h"

BEGIN_RADIANT_PLATFORM_NAMESPACE

class SysInfo 
{
public:
   SysInfo(std::string const& uri);
   ~SysInfo();

   typedef unsigned long long ByteCount;

   static std::string GetOSVersion();
   static std::string GetOSName();
   static ByteCount GetCurrentMemoryUsage();
   static void GetVirtualAddressSpaceUsage(ByteCount& total, ByteCount& avail);
   static ByteCount GetTotalSystemMemory();
   static void LogMemoryStatistics(const char* reason, uint level);
};

END_RADIANT_PLATFORM_NAMESPACE

#endif //  _RADIANT_PLATFORM_SYSINFO_H
