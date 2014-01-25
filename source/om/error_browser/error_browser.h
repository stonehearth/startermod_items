#ifndef _RADIANT_OM_ERROR_BROWSER_H
#define _RADIANT_OM_ERROR_BROWSER_H

#include <libjson.h>
#include "../namespace.h"
#include "om/om.h"
#include "om/json.h"
#include "om/macros.h"
#include "om/object_enums.h"
#include "dm/set.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "lib/json/node.h"

BEGIN_RADIANT_OM_NAMESPACE

class ErrorBrowser : public dm::Record
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
      Record& SetFileType(std::string const& s);

      json::Node const& GetNode() const { return node_; }

   private:
      json::Node     node_;
   };

public:
   DEFINE_OM_OBJECT_TYPE(ErrorBrowser, error_browser);
   void AddRecord(Record const& r);

   dm::Set<std::shared_ptr<JsonBoxed>> const& GetEntries() const { return entries_; }

private:
   void InitializeRecordFields() override;

private:
   dm::Set<std::shared_ptr<JsonBoxed>>    entries_;
   int                                    next_record_id_;
};

std::ostream& operator<<(std::ostream &os, ErrorBrowser::Record const& r);

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ERROR_BROWSER_H
