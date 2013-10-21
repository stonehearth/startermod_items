#ifndef _RADIANT_JSON_NAMESPACE_H
#define _RADIANT_JSON_NAMESPACE_H

#define BEGIN_RADIANT_JSON_NAMESPACE  namespace radiant { namespace json {
#define END_RADIANT_JSON_NAMESPACE    } }

#include "macros.h"

BEGIN_RADIANT_JSON_NAMESPACE

class InvalidJson : public std::invalid_argument {
public:
   InvalidJson(std::string const& s) : std::invalid_argument(s) { }
};

class Node;
template <typename T> T decode(Node const&);
template <typename T> Node encode(T const&);

template <typename T> T decode(Node const& node, T const& def)
{
   try {
      return decode<T>(node);
   } catch (InvalidJson const&) {
   }
   return def;
}

END_RADIANT_JSON_NAMESPACE

#endif //  _RADIANT_JSON_NAMESPACE_H
