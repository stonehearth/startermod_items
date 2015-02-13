#ifndef _RADIANT_CORE_SYSTEM_H
#define _RADIANT_CORE_SYSTEM_H

#include <boost/filesystem.hpp>
#include "singleton.h"

BEGIN_RADIANT_CORE_NAMESPACE

class System : public Singleton<System>
{
public:
   System();
   
   boost::filesystem::path GetTempDirectory() const;
   static bool IsProcess64Bit();
   static bool IsPlatform64Bit();
   static void Log(std::string const& str) { Log(str.c_str()); }
   static void Log(const char* src);

private:
   NO_COPY_CONSTRUCTOR(System);

   bool IsAncestorDirectory(boost::filesystem::path const& ancestor, boost::filesystem::path const& directory) const;
   boost::filesystem::path DefaultTempDirectory(std::string const& application_name) const;
   bool IsWritableDirectory(std::string const& directory) const;

   boost::filesystem::path temp_directory_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_SYSTEM_H
