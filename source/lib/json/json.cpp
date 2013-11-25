#include "radiant.h"
#include "radiant_exceptions.h"
#include "radiant_macros.h"
#include "json.h"

using namespace radiant;
using namespace radiant::json;

static void JsonParseErrorCallback(json_string const& error_message)
{
   throw json::InvalidJson(libjson::to_std_string(error_message).c_str());
}

void json::InitialzeErrorHandler()
{
   libjson::register_debug_callback(JsonParseErrorCallback);
}

bool json::ReadJsonFile(std::string const& filename, JSONNode& dst_node, std::string& error_message)
{
   std::ifstream stream(filename);
   return ReadJson(stream, dst_node, error_message);
}

bool json::ReadJson(std::istream& stream, JSONNode& dst_node, std::string& error_message)
{
   std::stringstream reader;
   reader << stream.rdbuf();
   json_string text = reader.str();
   return ReadJson(text, dst_node, error_message);
}

bool json::ReadJson(json_string const& text, JSONNode& dst_node, std::string& error_message)
{
   try {
      JSONNode node = libjson::parse(text);
      // resolve all nodes to check for errors
      node.preparse();
      //node.write_formatted();
      error_message = std::string();
      dst_node = node;
      return true;
   } catch (std::exception const& e) {
      error_message = BUILD_STRING(e.what() << "\n\n" << "Source:\n" << text);
      return false;
   }
}
