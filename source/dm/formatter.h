#pragma once
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T>
struct Formatter {
   static std::ostream& Log(std::ostream& out, const T& value, std::string indent)
   {
      return (out << value);
   }
};

struct FormatData
{
   std::string indent;
   FormatData() { }
   FormatData(std::string i) : indent(i) { }
};

template <class T>
struct Format : public FormatData {
   Format(const T& o) : value(o) {}
   Format(const T& o, std::string i) : FormatData(i), value(o) {}

   const T& value;
};

template <class T>
std::ostream& operator<<(std::ostream& out, const Format<T>& o)
{
   return Formatter<T>::Log(out, o.value, o.indent);
}

END_RADIANT_DM_NAMESPACE

