#include "radiant.h"
#include "node.h"

using namespace radiant;
using namespace radiant::json;

Node::const_iterator Node::find(std::string const& path) const {
   size_t first = 0;
   size_t last = path.find('.', first + 1);

   // Fast path for non-dotted strings.
   if (last == std::string::npos) {
      return const_iterator(node_.find(path));
   }

   JSONNode::const_iterator last_i = node_.end();
   JSONNode::const_iterator last_end = node_.end();
   JSONNode::const_iterator current_i = node_.find(path.substr(first, last));
   for (;;) {
      if (current_i == last_end) {
         break;
      }
      if (last == std::string::npos) {
         break;
      }
      if (current_i->type() != JSON_NODE) {
         current_i = last_end;
         break;
      }
      first = last + 1;
      last = path.find('.', first + 1);
      last_i = current_i;
      last_end = last_i->end();

      std::string key = path.substr(first, last - first);
      current_i = last_i->find(key);
   }
   if (current_i == last_end) {
      return node_.end();
   }
   return const_iterator(current_i);
}

void Node::chop_path_front(std::string const& path, std::string& node_name, std::string& remaining_path) const
{
   size_t const offset = path.find(path_delimiter);
   if (offset != std::string::npos) {
      node_name = path.substr(0, offset);
      remaining_path = path.substr(offset + 1);
   } else {
      node_name = path;
      remaining_path = std::string();
   }
}

void Node::chop_path_back(std::string const& path, std::string& node_name, std::string& remaining_path) const
{
   size_t const offset = path.rfind(path_delimiter);
   if (offset != std::string::npos) {
      node_name = path.substr(offset + 1);
      remaining_path = path.substr(0, offset);
   } else {
      node_name = path;
      remaining_path = std::string();
   }
}

JSONNode Node::create_ancestors(std::string const& path, JSONNode& node) const
{
   std::string current_path, remaining_path, node_name;
   JSONNode current_node, parent_node;

   current_node = node;
   current_path = path;

   // Invariants
   // current_node = last node created
   // current_path = remaining path (ancestors) to process
   while (true) {
      chop_path_back(current_path, node_name, remaining_path);
      if (node_name.empty()) break;

      parent_node = JSONNode();
      parent_node.set_name(node_name);
      parent_node.push_back(current_node);

      current_node = parent_node;
      current_path = remaining_path;
   }

   return current_node;
}

template <> std::string json::decode(Node const& node) {
   if (node.type() == JSON_STRING) {
      return node.get_internal_node().as_string();
   }
   throw InvalidJson(BUILD_STRING("expected string value for node"));
}

template <> int json::decode(Node const& node) {
   if (node.type() == JSON_NUMBER) {
      return node.get_internal_node().as_int();
   }
   throw InvalidJson(BUILD_STRING("expected number value for node"));
}

template <> double json::decode(Node const& node) {
   if (node.type() == JSON_NUMBER) {
      return node.get_internal_node().as_float();
   }
   throw InvalidJson(BUILD_STRING("expected number value for node"));
}

template <> float json::decode(Node const& node) {
   return (float)decode<double>(node);
}

template <> bool json::decode(Node const& node) {
   if (node.type() == JSON_BOOL) {
      return node.get_internal_node().as_bool();
   }
   if (node.type() == JSON_NUMBER) {
      return node.get_internal_node().as_int() != 0;
   }
   if (node.type() == JSON_STRING) {
      return node.get_internal_node().as_string() == "true";
   }
   throw InvalidJson(BUILD_STRING("expected number bool for node"));
}

template <> Node json::decode(Node const& node) {
   return node;
}

template <> JSONNode json::decode(Node const& node) {
   return node.get_internal_node();
}


template <> Node json::encode(Node const& n) {
   return n;
}
