#ifndef _RADIANT_PLATFORM_SYSINFO_H
#define _RADIANT_PLATFORM_SYSINFO_H

#include "namespace.h"

BEGIN_RADIANT_PLATFORM_NAMESPACE

class SysInfo 
{
public:
   SysInfo(std::string const& uri);
   ~SysInfo();

   static std::string GetOSVersion();
   static std::string GetOSName();
};

END_RADIANT_PLATFORM_NAMESPACE

#endif //  _RADIANT_PLATFORM_SYSINFO_H
