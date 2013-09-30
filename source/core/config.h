#ifndef _RADIANT_CORE_CONFIG_H
#define _RADIANT_CORE_CONFIG_H

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "singleton.h"

BEGIN_RADIANT_CORE_NAMESPACE

class Config : public Singleton<Config>
{
public:
   Config();
   
   bool Load(std::string const &name, int argc, const char *argv[]);

   boost::program_options::options_description& GetConfigFileOptions();
   boost::program_options::options_description& GetCommandLineOptions();

   std::string GetName() const;
   boost::filesystem::path GetCacheDirectory() const;
   boost::filesystem::path GetTmpDirectory() const;

   boost::program_options::variables_map const& GetVarMap() const { return configvm_; }
   
private:
   void LoadConfigFile(boost::filesystem::path const& configfile);

private:
   std::string                                  name_;
   boost::program_options::options_description  cmd_line_options_;
   boost::program_options::options_description  config_file_options_;
   boost::program_options::variables_map        configvm_;
   std::string                                  config_filename_;

   boost::filesystem::path                      cache_directory_;
   boost::filesystem::path                      run_directory_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_CONFIG_H
