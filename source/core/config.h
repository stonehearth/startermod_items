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
      return root_config_node_.has(property_path);
   }

   template <class T>
   T GetProperty(std::string const& property_path) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_node_.get<T>(property_path);
   }

   template <class T>
   T GetProperty(std::string const& property_path, T const& default_value) const
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      return root_config_node_.get(property_path, default_value);
   }

   // Help the compiler with c string as a template type
   std::string GetProperty(std::string const& property_path, const char* const default_value) const
   {
      return GetProperty(property_path, std::string(default_value));
   }

   template <class T>
   void SetProperty(std::string const& property_path, T const& value, bool write_to_user_config = false)
   {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      root_config_node_.set(property_path, value);

      if (write_to_user_config) {
         // don't cache the user_config since this is an uncommon operation
         boost::filesystem::path const file_path = cache_directory_ / config_filename_;
         json::Node user_config = ReadConfigFile(file_path, false);
         user_config.set(property_path, value);
         WriteConfigFile(file_path, user_config);
      }
   }

   // Help the compiler with c string as a template type
   void SetProperty(std::string const& property_path, const char* const value, bool write_to_user_config = false)
   {
      SetProperty(property_path, std::string(value), write_to_user_config);
   }

   std::string GetName() const;
   boost::filesystem::path GetCacheDirectory() const;
   boost::filesystem::path GetTmpDirectory() const;

   std::string GetUserID() const;
   std::string GetSessionID() const;
   std::string GetBuildNumber() const;
   bool IsCollectionStatusSet() const;
   bool GetCollectionStatus() const;
   void SetCollectionStatus(bool should_collect);

private:
   Config(const Config&);              // Prevent copy-construction
   Config& operator=(const Config&);   // Prevent assignment

   json::Node ReadConfigFile(boost::filesystem::path const& file_path, bool throw_on_not_found = true) const;
   void WriteConfigFile(boost::filesystem::path const& file_path, json::Node const& config) const;
   boost::filesystem::path LookupCacheDirectory() const;
   void CmdLineParamToKeyValue(std::string const& param, std::string& key, std::string& value) const;
   json::Node ParseCommandLine(int argc, const char *argv[]) const;
   json::Node ParseJsonFile(boost::filesystem::path const& filepath) const;
   void MergeConfigNodes(JSONNode& base, JSONNode const& override) const;

private:
   std::string                    config_filename_;
   boost::filesystem::path        cache_directory_;
   boost::filesystem::path        run_directory_;
   json::Node                     root_config_node_;
   mutable std::recursive_mutex   mutex_;
   
   std::string                    sessionid_;
   std::string                    userid_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_CONFIG_H
