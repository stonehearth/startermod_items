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

      class ConstJsonObject {
      public:
         ConstJsonObject(JSONNode const& node) : node_(node) { }
         static void RegisterLuaType(struct lua_State* L);

         std::string ToString() const;
         JSONNode const& GetNode() const { return node_; }

         template <typename T> T get(char const* name, T const& def) const {
            return ::radiant::json::get(GetNode(), name, def);
         }
      private:
         JSONNode const& node_;
      };
      static std::ostream& operator<<(std::ostream& os, const ConstJsonObject& o) { return (os << o.ToString()); }


      template <typename T> static T cast_(JSONNode const& node);

      template <> static std::string cast_(JSONNode const& node) {
         return node.as_string();
      }
      template <> static float cast_(JSONNode const& node) {
         return static_cast<float>(node.as_float());
      }
      template <> static int cast_(JSONNode const& node) {
         return node.as_int();
      }
      template <> static JSONNode cast_(JSONNode const& node) {
         return node;
      }
   };
};

#endif // _RADIANT_JSON_H
