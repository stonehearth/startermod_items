#pragma once
#include <set>
#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Object;

class DbgInfo
{
public:
   static std::string GetInfoString(dm::Object const& obj);

   DbgInfo(std::ostream& o) : os(o) { }

   void GetInfo(dm::Object const& obj);
   void MarkVisited(dm::Object const& obj);
   bool IsVisisted(dm::Object const& obj) const;

   std::ostream&      os;


private:
   std::set<dm::ObjectId>     visisted_;
};

END_RADIANT_DM_NAMESPACE
