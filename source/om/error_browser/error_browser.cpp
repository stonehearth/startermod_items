#include "pch.h"
#include "dm/store.h"
#include "error_browser.h"

using namespace ::radiant;
using namespace ::radiant::om;

void ErrorBrowser::InitializeRecordFields()
{
   dm::Record::InitializeRecordFields();
   AddRecordField("entries", entries_);
   if (!IsRemoteRecord()) {
      next_record_id_ = 1;
   }
}

void ErrorBrowser::AddRecord(ErrorBrowser::Record const& r)
{
   JSONNode n = r.GetNode();
   n.push_back(JSONNode("id", stdutil::ToString(next_record_id_++)));

   auto entry = GetStore().AllocObject<dm::Boxed<json::Node>>();
   entry->Set(n);
   entries_.Add(entry);
}
