#include "pch.h"
#include "dm/store.h"
#include "om/json_store.h"
#include "error_browser.h"

using namespace ::radiant;
using namespace ::radiant::lib;

ErrorBrowser::ErrorBrowser(dm::Store& store) :
   next_record_id_(1)
{
   json_ = store.AllocObject<om::JsonStore>();
   json_->Modify() = JSONNode(JSON_ARRAY);
}

void ErrorBrowser::AddRecord(ErrorBrowser::Record const& r)
{
   JSONNode n = r.GetNode();
   n.push_back(JSONNode("id", stdutil::ToString(next_record_id_++)));

   JSONNode &json = json_->Modify();
   json.push_back(n);
}
