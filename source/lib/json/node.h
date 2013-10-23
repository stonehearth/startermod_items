#ifndef _RADIANT_JSON_NODE_H
#define _RADIANT_JSON_NODE_H

#include <sstream>
#include <libjson.h>
#include "namespace.h"

BEGIN_RADIANT_JSON_NAMESPACE

class Node {
public:
   Node() { }
   Node(JSONNode const& node) : node_(node) { }

   int type() const { return node_.type(); }
   int size() const { return node_.size(); }
   bool empty() const { return node_.empty(); }

   std::string write() const {
      return node_.write();
   }

   std::string write_formatted() const {
      return node_.write_formatted();
   }

   JSONNode const& GetNode() const {
      return node_;
   }

   operator JSONNode () const {
      return node_;
   }

   bool has(std::string const& name) const {
      return find(name) != end();
   }

   template <typename T> T as() const {
      return json::decode<T>(*this);
   }

   template <typename T> T as(T const& def) const {
      try {
         return as<T>();
      } catch (InvalidJson const&) {
      }
      return def;
   }

   template <typename T> T get(std::string const& name) const {
      auto i = find(name);
      if (i != end()) {
         return Node(*i).as<T>();
      }
      throw InvalidJson(BUILD_STRING("node has no key named " << name));
   }

   template <typename T> T get(std::string const& name, T const& def) const {
      try {
         auto i = find(name);
         if (i != end()) {
            return Node(*i).as<T>();
         }
      } catch (InvalidJson const&) {
      }
      return def;
   }

   template <typename T> T get(unsigned int index) const {
      ASSERT(node_.type() == JSON_ARRAY);
      if (index >= 0 && index < (int)node_.size()) {
         return Node(node_.at(index)).as<T>();
      }
      throw InvalidJson(BUILD_STRING("index " << index << " out-of-bounds"));
   }

   template <typename T> T get(unsigned int index, T const& def) const {
      try {
         if (index >= 0 && index < (int)node_.size()) {
            return Node(node_.at(index)).as<T>();
         }
      } catch (InvalidJson const&) {
      }
      return def;
   }

   Node getn(std::string const& name, Node const& def = Node()) const {
      return Node(get<JSONNode>(name, def));
   }

   Node getn(unsigned int index, Node const& def = Node()) const {
      return Node(get<JSONNode>(index, def));
   }

   template <typename T> JSONNode create_node(std::string const& name, T const& value) const {
      JSONNode node = encode(value);
      node.set_name(name);
      return node;
   }

   template <> JSONNode create_node(std::string const& name, int const& value) const {
      return JSONNode(name, value);
   }

   template <> JSONNode create_node(std::string const& name, bool const& value) const {
      return JSONNode(name, value);
   }

   template <> JSONNode create_node(std::string const& name, float const& value) const {
      return JSONNode(name, value);
   }

   template <> JSONNode create_node(std::string const& name, double const& value) const {
      return JSONNode(name, value);
   }

   template <> JSONNode create_node(std::string const& name, std::string const& value) const {
      return JSONNode(name, value);
   }

   template <> JSONNode create_node(std::string const& name, JSONNode const& value) const {
      JSONNode result = value;
      result.set_name(name);
      return result;
   }

   template <typename T> Node& set(std::string const& name, T const& value) {
      size_t offset = name.find('.');
      if (offset == std::string::npos) {
         auto i = node_.find(name);
         if (i != node_.end()) {
            *i = create_node(name, value);
         } else {
            node_.push_back(create_node(name, value));
         }
      } else {
         auto i = node_.find(name.substr(0, offset));
         if (i != node_.end()) {
            if (i->type() != JSON_NODE) {
               throw InvalidJson(BUILD_STRING("cannot set key " << name << " of non-node json value"));
            }     
            Node(*i).set(name.substr(offset + 1), value);
         } else {
            Node child;
            child.node_.set_name(name.substr(0, offset));
            child.set(name.substr(offset + 1), value);
            node_.push_back(child);
         }
      }
      return *this;
   }

   template <typename T> Node& add(T const& value) {
      node_.push_back(create_node("", value));
      return *this;
   }


   // xxx: these should return Node's, right?
   JSONNode::const_iterator begin() const { return node_.begin(); }
   JSONNode::const_iterator end() const { return node_.end(); }
   JSONNode::const_iterator find(std::string const& name) const {
      size_t offset = name.find('.');
      if (offset == std::string::npos) {
         return node_.find(name);
      }
      auto i = node_.find(name.substr(0, offset));
      if (i == node_.end()) {
         return i;
      }
      return Node(*i).find(name.substr(offset + 1));
   }

private:
   JSONNode node_;
};

template <> std::string decode(Node const& node);
template <> int decode(Node const& node);
template <> double decode(Node const& node);
template <> float decode(Node const& node);
template <> bool decode(Node const& node);
template <> Node decode(Node const& node);
template <> JSONNode decode(Node const& node);
template <> Node encode(Node const& n);

static std::ostream& operator<<(std::ostream& os, const Node& o) { return (os << o.write_formatted()); }

END_RADIANT_JSON_NAMESPACE

#endif // _RADIANT_JSON_NODE_H
