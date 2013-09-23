#ifndef _RADIANT_RES_MANIFEST_H
#define _RADIANT_RES_MANIFEST_H

#include <boost/filesystem.hpp>
#include "radiant_json.h"
#include "radiant_exceptions.h"
#include "namespace.h"

BEGIN_RADIANT_RES_NAMESPACE

#define DECLARE_OBJECT(Cls)  Cls(json::ConstJsonObject const& n) : json::ConstJsonObject(n) { }

#define DECLARE_MOD_OBJECT(Cls)                                      \
   public:                                                           \
      Cls(std::string const& mod, json::ConstJsonObject const& n) :  \
         mod_name_(mod),                                             \
         json::ConstJsonObject(n) {}                                 \
   private:                                                          \
      std::string mod_name_;                                         \
   public:                                                           \


struct FilePath
{
   FilePath(std::string const& path);

   operator bool();
   operator std::string();

private:
   std::string    path_;
};

struct Function : public json::ConstJsonObject
{
   Function(std::string const& name, json::ConstJsonObject const& n);

   static const std::string SERVER;

   std::string endpoint();
   FilePath script();

   operator bool();

private:
   std::string name_;
};


struct FunctionsBlock : public json::ConstJsonObject
{
   DECLARE_MOD_OBJECT(FunctionsBlock);

   Function get_function(std::string const& name);
};

struct Manifest : public json::ConstJsonObject
{
   DECLARE_MOD_OBJECT(Manifest);

   FunctionsBlock get_functions();
   Function get_function(std::string name);
};

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_RESOURCE_MANAGER_H
