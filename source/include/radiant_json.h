#ifndef _RADIANT_JSON_H
#define _RADIANT_JSON_H

#include "libjson.h"
#include <luabind/luabind.hpp>

namespace radiant {
   namespace json {
      template <typename T> static T get(JSONNode const& node, char const* name) {
         return get<T>(node, name, T());
      }

      template <typename T> static T get(JSONNode const& node, char const* name, T const& def) {
         auto i = node.find(name);
         if (i != node.end()) {
            return cast_<T>(*i);
         }
         return def;
      }

      template <typename T> static T get(JSONNode const& node, int index) {
         return get<T>(node, index, T());
      }

      template <typename T> static T get(JSONNode const& node, int index, T const& def) {
         ASSERT(node.type() == JSON_ARRAY);
         if (index >= 0 && index < (int)node.size()) {
            return cast_<T>(node.at(index));
         }
         return def;
      }

      class ConstJsonObject {
      public:
         ConstJsonObject(JSONNode const& node) : node_(node) { }
         static void RegisterLuaType(struct lua_State* L);

         std::string write() const {
            return node_.write();
         }

         std::string write_formatted() const {
            return node_.write_formatted();
         }

         int as_integer() const {
            ASSERT(node_.type() == JSON_NUMBER);
            return node_.as_int();
         }

         std::string as_string() const {
            ASSERT(node_.type() == JSON_STRING);
            return node_.as_string();
         }

         JSONNode const& GetNode() const { return node_; }

         bool has(const char* name) const {
            return node_.find(name) != node_.end();
         }

         template <typename T> T get(char const* name, T const& def) const {
            return ::radiant::json::get(GetNode(), name, def);
         }
         template <typename T> T get(int offset, T const& def) const {
            return ::radiant::json::get(GetNode(), offset, def);
         }
         ConstJsonObject operator[](std::string const& name) const {
            return (*this)[name.c_str()];
         }

         ConstJsonObject operator[](const char* name) const {
            ASSERT(name);
            ASSERT(node_.type() == JSON_NODE);

            auto i = node_.find(name);
            ASSERT(i != node_.end());

            return ConstJsonObject(*i);
         }

         ConstJsonObject operator[](int i) const {
            ASSERT(i >= 0);
            return (*this)[(unsigned int)i];
         }

         ConstJsonObject operator[](unsigned int i) const {
            ASSERT(node_.type() == JSON_ARRAY);
            ASSERT(i < node_.size());
            return node_.at(i);
         }
      private:
         JSONNode const& node_;
      };
      static std::ostream& operator<<(std::ostream& os, const ConstJsonObject& o) { return (os << o.write_formatted()); }


      template <typename T> static T cast_(JSONNode const& node);

      template <> static JSONNode cast_(JSONNode const& node) {
         return node;
      }
      template <> static float cast_(JSONNode const& node) {
         return static_cast<float>(node.as_float());
      }
      template <> static int cast_(JSONNode const& node) {
         return node.as_int();
      }
      template <> static std::string cast_(JSONNode const& node) {
         return node.as_string();
      }
   };
};

#endif // _RADIANT_JSON_H
