#include "radiant.h"
#include "node.h"

using namespace radiant;
using namespace radiant::json;

template <> std::string json::decode(Node const& node) {
   if (node.type() == JSON_STRING) {
      return node.GetNode().as_string();
   }
   throw InvalidJson(BUILD_STRING("expected string value for node"));
}

template <> int json::decode(Node const& node) {
   if (node.type() == JSON_NUMBER) {
      return node.GetNode().as_int();
   }
   throw InvalidJson(BUILD_STRING("expected number value for node"));
}

template <> double json::decode(Node const& node) {
   if (node.type() == JSON_NUMBER) {
      return node.GetNode().as_float();
   }
   throw InvalidJson(BUILD_STRING("expected number value for node"));
}

template <> float json::decode(Node const& node) {
   return (float)decode<double>(node);
}

template <> bool json::decode(Node const& node) {
   if (node.type() == JSON_BOOL) {
      return node.GetNode().as_bool();
   }
   if (node.type() == JSON_NUMBER) {
      return node.GetNode().as_int() != 0;
   }
   if (node.type() == JSON_STRING) {
      return node.GetNode().as_string() == "true";
   }
   throw InvalidJson(BUILD_STRING("expected number bool for node"));
}

template <> Node json::decode(Node const& node) {
   return node;
}

template <> JSONNode json::decode(Node const& node) {
   return node.GetNode();
}


template <> Node json::encode(Node const& n) {
   return n;
}
