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

   template <class T>
   Node(std::string const& name, T const& value) : node_(JSONNode(name, value)) { }

   std::string name() const { return node_.name(); }
   void set_name(std::string const& name) { return node_.set_name(name); }
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
      std::string current_path, next_path, next_node_name;
      JSONNode const* node = &node_;

      current_path = path;

      try {
         while (true) {
            if (!is_nested_path(current_path)) {
               // no nesting, just get it
               auto i = node->find(current_path);

               if (i != node->end()) {
                  // replace existing value
                  return Node(*i).as<T>();
               }
               throw InvalidJson(BUILD_STRING("JSON node has no key named '" << current_path << "'."));
            }

            chop_path_front(current_path, next_node_name, next_path);
            auto i = node->find(next_node_name);

            if (i == node->end()) {
               throw InvalidJson(BUILD_STRING("JSON node '" << next_node_name << "' not found."));
            }

            // path exists, make sure it's a node and not a property
            if (i->type() != JSON_NODE) {
               throw InvalidJson(BUILD_STRING("Cannot navigate through '" << next_node_name << "'"));
            }

            current_path = next_path;
            node = &*i;
         }
      } catch (std::exception const& e) {
         throw InvalidJson(BUILD_STRING("Error getting path '" << path << "':\n\n" << e.what()));
      }
   }

   template <typename T> T get(std::string const& path, T const& default_value) const {
      std::string current_path, next_path, next_node_name;
      JSONNode const* node = &node_;

      current_path = path;

      while (true) {
         if (!is_nested_path(current_path)) {
            // no nesting, just get it
            auto i = node->find(current_path);

            if (i != node->end()) {
               // replace existing value
               return Node(*i).as<T>();
            }
            return default_value;
         }

         chop_path_front(current_path, next_node_name, next_path);
         auto i = node->find(next_node_name);

         if (i == node->end()) {
            return default_value;
         }

         // path exists, make sure it's a node and not a property
         if (i->type() != JSON_NODE) {
            return default_value;
         }

         current_path = next_path;
         node = &*i;
      }
   }

   std::string get(std::string const& path, char const* const default_value) const {
      return get(path, std::string(default_value));
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

   Node get_node(std::string const& name, Node const& default = Node()) const {
      return Node(get<JSONNode>(name, default));
   }

   Node get_node(unsigned int index, Node const& default = Node()) const {
      return Node(get<JSONNode>(index, default));
   }

   template <typename T> Node& set(std::string const& path, T const& value) {
      std::string current_path, next_path, next_node_name;
      JSONNode* node = &node_;

      current_path = path;

      try {
         while (true) {
            if (!is_nested_path(current_path)) {
               // no nesting, just set it
               auto i = node->find(current_path);

               if (i != node->end()) {
                  // replace existing value
                  *i = create_node(current_path, value);
               } else {
                  // add new node
                  node->push_back(create_node(current_path, value));
               }
               return *this;
            }

            chop_path_front(current_path, next_node_name, next_path);
            auto i = node->find(next_node_name);

            if (i == node->end()) {
               // node doesn't exist, create and assign the subgraph
               node->push_back(create_node(current_path, value));
               return *this;
            }

            // path exists, make sure it's a node and not a property
            if (i->type() != JSON_NODE) {
               throw InvalidJson(BUILD_STRING("Cannot navigate through '" << next_node_name << "'"));
            }

            current_path = next_path;
            node = &*i;
         }
      } catch (std::exception const& e) {
         throw InvalidJson(BUILD_STRING("Error setting path '" << path << "':\n\n" << e.what()));
      }
   }

   Node& add_node(Node const& new_node)
   {
      node_.push_back(new_node.node_);
      return *this;
   }

   template <typename T> Node& add(std::string const& name, T const& value) {
      node_.push_back(create_node(name, value));
      return *this;
   }

   template <typename T> Node& add(T const& value) {
      node_.push_back(create_node("", value));
      return *this;
   }

   class const_iterator;

   const_iterator begin() const { return const_iterator(node_.begin()); }
   const_iterator end() const { return const_iterator(node_.end()); }
   const_iterator find(std::string const& path) const {
      if (!is_nested_path(path)) {
         return const_iterator(node_.find(path));
      }
      std::string next_node_name, next_path;
      chop_path_front(path, next_node_name, next_path);

      auto i = node_.find(next_node_name);
      if (i == node_.end()) {
         return const_iterator(i);
      }
      return Node(*i).find(next_path);
   }

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
