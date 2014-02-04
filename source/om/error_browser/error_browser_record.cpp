#include "pch.h"
#include "dm/store.h"
#include "error_browser.h"

using namespace ::radiant;
using namespace ::radiant::om;

typedef ErrorBrowser::Record Record;

static const char *categories__[] = {
   "info",
   "warning",
   "severe",
};

std::ostream& om::operator<<(std::ostream &os, Record const& r)
{
   os << r.GetNode().write_formatted();
   return os;
}

Record::Record()
{
   SetCategory(INFO);
}

Record& Record::SetSummary(std::string const& s)
{
   node_.set("summary", s);
   return *this;
}

Record& Record::SetFileType(std::string const& s)
{
   node_.set("file_type", s);
   return *this;
}

Record& Record::SetBacktrace(std::string const& s)
{
   node_.set("backtrace", s);
   return *this;
}

Record& Record::SetLineNumber(int n)
{
   node_.set("line_number", n);
   return *this;
}

Record& Record::SetCharOffset(int n)
{
   node_.set("char_offset", n);
   return *this;
}

Record& Record::SetCategory(Category c)
{
   if (c >= 0 && c < ARRAYSIZE(categories__)) {
      node_.set("category", categories__[c]);
   }
   return *this;
}

Record& Record::SetFilename(std::string const &s)
{
   node_.set("filename", s);
   return *this;
}

Record& Record::SetFileContent(std::string const &s)
{
   node_.set("file_content", s);
   return *this;
}

