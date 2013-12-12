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

   std::string name() const { return node_.name(); }
   int type() const { return node_.type(); }
   int size() const { return node_.size(); }
   bool empty() const { return node_.empty(); }

   std::string write() const {
      return node_.write();
   }

   std::string write_formatted() const {
      return node_.write_formatted();
   }

   JSONNode const& get_internal_node() const {
      return node_;
   }

   operator JSONNode () const {
      return node_;
   }

   bool has(std::string const& path) const {
      return find(path) != end();
   }

   template <typename T> T as() const {
      return json::decode<T>(*this);
   }

   template <typename T> T as(T const& default_value) const {
      try {
         return as<T>();
      } catch (InvalidJson const&) {
      }
      return default_value;
   }

   template <typename T> T get(std::string const& path) const {
      auto i = find(path);
      if (i == end()) {
         throw InvalidJson(BUILD_STRING("Error getting path '" << path << "'."));
      }
      return (*i).as<T>();
   }

   template <typename T> T get(std::string const& path, T const& default_value) const {
      auto i = find(path);
      if (i == end()) {
         return default_value;
      }
      return (*i).as<T>(default_value);
   }

   template <typename T> T get(unsigned int index) const {
      ASSERT(node_.type() == JSON_ARRAY);
      if (index >= 0 && index < (int)node_.size()) {
         return Node(node_.at(index)).as<T>();
      }
      throw InvalidJson(BUILD_STRING("index " << index << " out-of-bounds."));
   }

   template <typename T> T get(unsigned int index, T const& default_value) const {
      try {
         if (index >= 0 && index < (int)node_.size()) {
            return Node(node_.at(index)).as<T>();
         }
      } catch (InvalidJson const&) {
      }
      return default_value;
   }

   // help the compiler with default values that are string literals
   std::string get(std::string const& path, char const* const default_value) const {
      return get(path, std::string(default_value));
   }

   Node get_node(std::string const& name, Node const& default = Node()) const {
      return Node(get<JSONNode>(name, default));
   }

   Node get_node(unsigned int index, Node const& default = Node()) const {
      return Node(get<JSONNode>(index, default));
   }

   // add is use to build arrays.  do not use set!
   template <typename T> Node& add(T const& value) {
      ASSERT(node_.type() == JSON_ARRAY);
      node_.push_back(encode(value));
      return *this;
   }

   // set is used to modify associative arrays.  do not use add!
   template <typename T> Node& set(std::string const& path, T const& value) {
      ASSERT(node_.type() == JSON_NODE);

      std::string current_path, next_path, next_node_name;
      JSONNode* node = &node_;

      current_path = path;

      try {
         while (true) {
            chop_path_front(current_path, next_node_name, next_path);
            auto i = node->find(next_node_name);

            if (i == node->end()) {
               // node doesn't exist, create and assign the subgraph
               node->push_back(create_node(current_path, value));
               return *this;
            }
            if (next_path.empty()) {
               // node found, replace existing value
               *i = create_node(current_path, value);
               return *this;
            }

            // path exists, make sure it's a node and not a property
            if (i->type() != JSON_NODE) {
               throw InvalidJson(BUILD_STRING("Cannot navigate through '" << next_node_name << "'"));
            }

            // iterate to the next node
            current_path = next_path;
            node = &*i;
         }
      } catch (std::exception const& e) {
         throw InvalidJson(BUILD_STRING("Error setting path '" << path << "':\n\n" << e.what()));
      }
   }

   // help the compiler with values that are string literals
   Node& set(std::string const& path, char const* const value) {
      return set(path, std::string(value));
   }

   class const_iterator;

   const_iterator begin() const { return const_iterator(node_.begin()); }
   const_iterator end() const { return const_iterator(node_.end()); }

   class const_iterator {
   public:
      inline const_iterator(JSONNode::const_iterator it) : it_(it) {}
      inline const_iterator(const_iterator const& rhs) : it_(rhs.it_) {}
      inline const_iterator& operator=(const_iterator const& rhs) { it_ = rhs.it_; return *this; }

      inline Node const operator*() const { return Node(*it_); }

      inline bool operator!=(const_iterator const& rhs) const { return it_ != rhs.it_; }
      inline bool operator==(const_iterator const& rhs) const { return it_ == rhs.it_; }

      inline const_iterator& operator++() { ++it_; return *this; }
      inline const_iterator& operator--() { --it_; return *this; }

      inline const_iterator operator++(int) { const_iterator result(*this); ++it_; return result; }
      inline const_iterator operator--(int) { const_iterator result(*this); --it_; return result; }

   private:
      // not supported - use operator* instead
      Node const* operator->() const;

      JSONNode::const_iterator it_;
   };

private:
   void set_name(std::string const& name) { return node_.set_name(name); }
   const_iterator find(std::string const& path) const;

   template <typename T> JSONNode create_node(std::string const& path, T const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      JSONNode node = (JSONNode)encode(value);
      node.set_name(node_name);
      return create_ancestors(remaining_path, node);
   }

   template <> JSONNode create_node(std::string const& path, int const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      return create_ancestors(remaining_path, JSONNode(node_name, value));
   }

   template <> JSONNode create_node(std::string const& path, bool const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      return create_ancestors(remaining_path, JSONNode(node_name, value));
   }

   template <> JSONNode create_node(std::string const& path, float const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      return create_ancestors(remaining_path, JSONNode(node_name, value));
   }

   template <> JSONNode create_node(std::string const& path, double const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      return create_ancestors(remaining_path, JSONNode(node_name, value));
   }

   template <> JSONNode create_node(std::string const& path, std::string const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      return create_ancestors(remaining_path, JSONNode(node_name, value));
   }

   template <> JSONNode create_node(std::string const& path, char* const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      return create_ancestors(remaining_path, JSONNode(node_name, value));
   }

   template <> JSONNode create_node(std::string const& path, JSONNode const& value) const {
      std::string remaining_path, node_name;
      chop_path_back(path, node_name, remaining_path);
      JSONNode node(value);
      node.set_name(node_name);
      return create_ancestors(remaining_path, node);
   }

   bool is_nested_path(std::string const& path) const;
   void chop_path_front(std::string const& path, std::string& node_name, std::string& remaining_path) const;
   void chop_path_back(std::string const& path, std::string& node_name, std::string& remaining_path) const;
   JSONNode create_ancestors(std::string const& path, JSONNode& node) const;

   static char const path_delimiter = '.';
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
