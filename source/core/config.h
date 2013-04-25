#ifndef _RADIANT_CORE_CONFIG_H
#define _RADIANT_CORE_CONFIG_H

#include "singleton.h"

BEGIN_RADIANT_CORE_NAMESPACE

class Config : public Singleton<Config>
{
public:
   static void Load(std::string path) { Config::GetInstance().LoadConfig(path); }
   static void Load(char *argv[], int argc) { Config::GetInstance().LoadCommandLine(argv, argc); }

private:
   void LoadConfig(std::string path);
   void LoadCommandLine(char *argv[], int argc);
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_CONFIG_H
