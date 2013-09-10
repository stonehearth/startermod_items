#include "pch.h"
#include "dm/store.h"
#include "error_browser.h"

using namespace ::radiant;
using namespace ::radiant::om;

typedef ErrorBrowser::Record Record;

std::ostream& om::operator<<(std::ostream &os, Record const& r)
{
   os << "error_record {" << std::endl;
   for (auto const& entry : r.GetNode()) {
      os << "  " << entry.name() << " : ";
      if (entry.type() == JSON_NUMBER) {
         os << entry.as_int();
      } else {
         os << entry.as_string();
      }
      os << std::endl;
   }
   os << "}" << std::endl;
   return os;
}

Record::Record()
{
   SetCategory(INFO);
}

Record& Record::SetSummary(std::string const& s)
{
   node_.push_back(JSONNode("summary", s));
   return *this;
}

Record& Record::SetFileType(std::string const& s)
{
   node_.push_back(JSONNode("file_type", s));
   return *this;
}

Record& Record::SetBacktrace(std::string const& s)
{
   node_.push_back(JSONNode("backtrace", s));
   return *this;
}

Record& Record::SetLineNumber(int n)
{
   node_.push_back(JSONNode("line_number", n));
   return *this;
}

Record& Record::SetCharOffset(int n)
{
   node_.push_back(JSONNode("char_offset", n));
   return *this;
}

Record& Record::SetCategory(Category c)
{
   static const char *categories[] = {
      "info",
      "warning",
      "severe",
   };
   if (c >= 0 && c < ARRAYSIZE(categories)) {
      node_.push_back(JSONNode("category", categories[c]));
   }
   return *this;
}

Record& Record::SetFilename(std::string const &s)
{
   node_.push_back(JSONNode("filename", s));
   return *this;
}

Record& Record::SetFileContent(std::string const &s)
{
   node_.push_back(JSONNode("file_content", s));
   return *this;
}

