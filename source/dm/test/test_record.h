#pragma once   

#include "record.h"
#include "boxed.h"

class RecordTest : public Record
{
public:
   void InitializeRecordFields() override
   {
      AddRecordField(ivalue0);
      AddRecordField(ivalue1);
   }

   bool operator==(const RecordTest& other) const {
      return ivalue0 == other.ivalue0 && ivalue1 == other.ivalue1;
   }
   struct Hash { 
      size_t operator()(const RecordTest& o) const {
         return std::hash<int>()(o.ivalue0) + std::hash<int>()(o.ivalue1); 
      }
   };
   
public:
   Boxed<int>     ivalue0;
   Boxed<int>     ivalue1;
};



