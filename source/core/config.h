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

   bool HasProperty(std::string const& property_path) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_.has(property_path);
   }

   template <class T>
   T GetProperty(std::string const& property_path) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_.get<T>(property_path);
   }

   template <class T>
   T GetProperty(std::string const& property_path, T const& default_value) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_.get(property_path, default_value);
   }

   std::string GetProperty(std::string const& property_path, const char* const default_value) const
   {
      return GetProperty(property_path, std::string(default_value));
   }

   template <class T>
   void SetProperty(std::string const& property_path, T const& value, bool write_to_user_config = false)
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      root_config_.set(property_path, value);

      if (write_to_user_config) {
         // don't cache the user_config since this is an uncommon operation
         json::Node user_config = ReadConfigFile(override_config_filename_, false);
         user_config.set(property_path, value);
         WriteConfigFile(override_config_filename_, user_config);
      }
   }

   void SetProperty(std::string const& property_path, const char* const value, bool write_to_user_config = false)
   {
      SetProperty(property_path, std::string(value), write_to_user_config);
   }

   std::string GetName() const;
   boost::filesystem::path GetCacheDirectory() const;
   boost::filesystem::path GetTempDirectory() const;

   std::string GetUserID() const;
   std::string GetSessionID() const;
   std::string GetBuildNumber() const;

private:
   Config(const Config&);              // Prevent copy-construction
   Config& operator=(const Config&);   // Prevent assignment

   boost::filesystem::path DefaultCacheDirectory() const;
   void CmdLineOptionToKeyValue(std::string const& param, std::string& key, std::string& value) const;
   json::Node ParseCommandLine(int argc, const char *argv[]) const;
   json::Node ReadConfigFile(boost::filesystem::path const& file_path, bool throw_on_not_found = true) const;
   void WriteConfigFile(boost::filesystem::path const& file_path, json::Node const& config) const;
   void MergeConfigNodes(JSONNode& base, JSONNode const& override) const;

private:
   std::string const              config_filename_;
   boost::filesystem::path        run_directory_;
   boost::filesystem::path        base_config_filename_;
   boost::filesystem::path        cache_directory_;
   boost::filesystem::path        override_config_filename_;
   json::Node                     root_config_;
   mutable std::recursive_mutex   mutex_;
   std::string                    sessionid_;
   std::string                    userid_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_CONFIG_H
