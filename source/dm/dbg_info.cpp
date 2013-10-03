#include "pch.h"
#include <sstream>
#include "object.h"
#include "dbg_info.h"

using namespace ::radiant;
using namespace ::radiant::dm;

std::string DbgInfo::GetInfoString(dm::Object const& obj) {
   std::ostringstream os;

   dm::DbgInfo dinfo(os);
   dinfo.GetInfo(obj);
   return os.str();
}


void DbgInfo::GetInfo(dm::Object const& obj)
{
   visisted_.clear();
   obj.GetDbgInfo(*this);
};

void DbgInfo::MarkVisited(dm::Object const& obj)
{
   visisted_.insert(obj.GetObjectId());
}

bool DbgInfo::IsVisisted(dm::Object const& obj) const
{
   auto begin = visisted_.begin(), end = visisted_.end();
   return std::find(begin, end, obj.GetObjectId()) != end;
}

