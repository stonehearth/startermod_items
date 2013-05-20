#ifndef _RADIANT_JSON_H
#define _RADIANT_JSON_H

#include "libjson.h"
namespace radiant {
   class json {
   public:
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

   private:
      template <typename T> static T cast_(JSONNode const& node);

      template <> static std::string cast_(JSONNode const& node) {
         return node.as_string();
      }
      template <> static float cast_(JSONNode const& node) {
         return static_cast<float>(node.as_float());
      }
      template <> static JSONNode cast_(JSONNode const& node) {
         return node;
      }
   };
};

#endif // _RADIANT_JSON_H
