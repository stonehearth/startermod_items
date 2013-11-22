#include "radiant.h"
#include "manifest.h"

namespace fs = ::boost::filesystem;

using namespace ::radiant;
using namespace ::radiant::res;

FilePath::FilePath(std::string const& path) :
   path_(path)
{
}

FilePath::operator bool()
{
   auto path = ::boost::filesystem::path(path_);
   return !path_.empty() && ::boost::filesystem::is_regular_file(path);
};

FilePath::operator std::string() {
   return path_;
}

Function::Function(std::string const& name, json::Node const& n) :
   name_(name),
   json::Node(n)
{
}

std::string Function::endpoint()
{
   return get<std::string>("endpoint");
}

FilePath Function::script()
{
   return FilePath(get<std::string>("controller"));
}

Function::operator bool() {
   return !get_internal_node().empty();
}


Function FunctionsBlock::get_function(std::string const& name)
{
   return Function(mod_name_, get_node(name));
}

FunctionsBlock Manifest::get_functions()
{
   return FunctionsBlock(mod_name_, get_node("radiant").get_node("functions"));
}

Function Manifest::get_function(std::string name)
{
   return get_functions().get_function(name);
}

