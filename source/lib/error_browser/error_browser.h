#ifndef _RADIANT_OM_ERROR_BROWSER_H
#define _RADIANT_OM_ERROR_BROWSER_H

#include <libjson.h>
#include "../namespace.h"
#include "om/om.h"

BEGIN_RADIANT_LIB_NAMESPACE

class ErrorBrowser
{
public:
   class Record {
   public:
      Record();

      enum Category {
         INFO = 0,
         WARNING = 1,
         SEVERE = 2,
      };

   public:
      Record& SetCategory(Category c);
      Record& SetSummary(std::string const& s);
      Record& SetLineNumber(int n);
      Record& SetCharOffset(int n);
      Record& SetFilename(std::string const &s);
      Record& SetFileContent(std::string const &s);
      Record& SetBacktrace(std::string const& s);

      JSONNode const& GetNode() const { return node_; }

   private:
      JSONNode         node_;
   };

public:
   ErrorBrowser(dm::Store& store);

   void AddRecord(Record const& r);
   om::JsonStorePtr GetJsonStoreObject() const { return json_; }

private:
   om::JsonStorePtr     json_;
   int                  next_record_id_;
};

std::ostream& operator<<(std::ostream &os, ErrorBrowser::Record const& r);

END_RADIANT_LIB_NAMESPACE

#endif //  _RADIANT_OM_ERROR_BROWSER_H
