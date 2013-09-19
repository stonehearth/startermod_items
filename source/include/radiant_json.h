#ifndef _RADIANT_JSON_H
#define _RADIANT_JSON_H

#include <libjson.h>
#include <luabind/luabind.hpp>

namespace radiant {
   namespace json {
      class Exception : public std::exception {
      public:
         Exception(std::string const& error) : error_(error) { }
         const char* what() const override { return error_.c_str(); }
      private:
         std::string    error_;
      };

      class ConstJsonObject;
      template <typename T> T cast(ConstJsonObject const&, T const& def);

      class ConstJsonObject {
      public:
         ConstJsonObject(JSONNode const& node) : node_(node) { }
         static void RegisterLuaType(struct lua_State* L);

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

         bool has(std::string const& name) const {
            return node_.find(name) != node_.end();
         }

         template <typename T> T as(T const& def = T()) const {
            return json::cast<T>(*this, def);
         }

         template <> std::string as(std::string const& def) const {
            return (node_.type() == JSON_STRING) ? node_.as_string() : def;
         }

         template <> int as(int const& def) const {
            return (node_.type() == JSON_NUMBER) ? node_.as_int() : def;
         }

         template <> float as(float const& def) const {
            return (float)as<double>(def);
         }

         template <> double as(double const& def) const {
            return (node_.type() == JSON_NUMBER) ? node_.as_float() : def;
         }

         template <> JSONNode as(JSONNode const& def) const {
            return node_;
         }
         
         template <> bool as(bool const& def) const {
            if (node_.type() == JSON_BOOL) {
               return node_.as_bool();
            }
            if (node_.type() == JSON_NUMBER) {
               return node_.as_int() != 0;
            }
            if (node_.type() == JSON_STRING) {
               return node_.as_string() == "true";
            }
            return def;
         }

         template <typename T> T get(std::string const& name, T const& def = T()) const {
            auto i = node_.find(name);
            if (i != node_.end()) {
               return ConstJsonObject(*i).as<T>(def);
            }
            return def;
         }

         template <typename T> T get(unsigned int index, T const& def = T()) const {
            ASSERT(node_.type() == JSON_ARRAY);
            if (index >= 0 && index < (int)node_.size()) {
               return ConstJsonObject(node_.at(index)).as<T>(def);
            }
            return def;
         }

         ConstJsonObject getn(std::string const& name) const {
            return ConstJsonObject(get<JSONNode>(name));
         }
         ConstJsonObject getn(unsigned int index) const {
            return ConstJsonObject(get<JSONNode>(index));
         }

         // xxx: these shoudl return ConstJsonObject's, right?
         JSONNode::const_iterator begin() const { return node_.begin(); }
         JSONNode::const_iterator end() const { return node_.end(); }
         JSONNode::const_iterator find(std::string const& name) const { return node_.find(name); }

      private:
         JSONNode node_;
      };
      static std::ostream& operator<<(std::ostream& os, const ConstJsonObject& o) { return (os << o.write_formatted()); }
   };
};

#endif // _RADIANT_JSON_H
