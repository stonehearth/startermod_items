#ifndef _RADIANT_JSON_MACROS_H
#define _RADIANT_JSON_MACROS_H

#define DEFINE_INVALID_JSON_CONVERSION(T) \
   template <> json::Node json::encode(T const& node) { \
      throw std::invalid_argument(BUILD_STRING("cannot convert " << GetShortTypeName<T>() << " to json")); \
   }

#define DECLARE_JSON_CODEC_TEMPLATES(T)    \
   template <> T decode(Node const& node, T const& def); \
   template <> Node encode(T const& node);

#endif // _RADIANT_JSON_MACROS_H
