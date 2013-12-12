#ifndef _RADIANT_CORE_CONFIG_H
#define _RADIANT_CORE_CONFIG_H

#include <mutex>
#include <boost/filesystem.hpp>
#include "singleton.h"
#include "lib/json/node.h"

BEGIN_RADIANT_CORE_NAMESPACE

class Config : public Singleton<Config>
{
public:
   Config();
   
   void Load(int argc, const char *argv[]);

   bool Has(std::string const& property_path) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_.has(property_path);
   }

   template <class T>
   T Get(std::string const& property_path) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_.get<T>(property_path);
   }

   template <class T>
   T Get(std::string const& property_path, T const& default_value) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_.get(property_path, default_value);
   }

   // help the compiler with default values that are string literals
   std::string Get(std::string const& property_path, const char* const default_value) const
   {
      return Get(property_path, std::string(default_value));
   }

   template <class T>
   void Set(std::string const& property_path, T const& value)
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      root_config_.set(property_path, value);

      // write to the user config
      // don't cache the user config since this is an uncommon operation
      json::Node user_config;
      if (boost::filesystem::exists(override_config_file_path_)) {
         user_config = ReadConfigFile(override_config_file_path_);
      }
      user_config.set(property_path, value);
      WriteConfigFile(override_config_file_path_, user_config);
   }

   std::string GetUserID() const;
   std::string GetSessionID() const;
   std::string GetBuildNumber() const;

private:
   NO_COPY_CONSTRUCTOR(Config);

   void InitializeSession();
   json::Node ParseCommandLine(int argc, const char *argv[]) const;
   json::Node ReadConfigFile(boost::filesystem::path const& file_path) const;
   void WriteConfigFile(boost::filesystem::path const& file_path, json::Node const& config) const;
   void MergeConfigNodes(JSONNode& base, JSONNode const& override) const;
   void ParseCmdLineOption(json::Node& node, std::string const& param) const;

private:
   std::string const              config_filename_;
   boost::filesystem::path        base_config_file_path_;
   boost::filesystem::path        override_config_file_path_;
   boost::filesystem::path        temp_directory_;
   json::Node                     root_config_;
   mutable std::recursive_mutex   mutex_;
   std::string                    sessionid_;
   std::string                    userid_;
   std::string                    game_script_;
   std::string                    game_mod_;
   bool                           should_skip_title_screen_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_CONFIG_H
