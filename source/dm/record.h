#ifndef _RADIANT_DM_RECORD_H
#define _RADIANT_DM_RECORD_H

#include "object.h"

BEGIN_RADIANT_DM_NAMESPACE

class Record : public Object
{
public:
   Record();
   DEFINE_DM_OBJECT_TYPE(Record, record);
   std::ostream& GetDebugDescription(std::ostream& os) const {
      return (os << "record");
   }

   void GetDbgInfo(DbgInfo &info) const override;
   void LoadValue(SerializationType r, Protocol::Value const& msg) override;
   void SaveValue(SerializationType r, Protocol::Value* msg) const override;

   void ConstructObject() override;
   
   virtual void InitializeRecordFields() = 0;

   bool IsRemoteRecord() const { return slave_; }
   std::vector<std::pair<std::string, ObjectId>> const& GetFields() const { return fields_; }

protected:
   friend void dm::LoadObject(Record& record, Protocol::Value const& msg);

   bool FieldIsUnique(std::string const& name, Object& field);
   void AddRecordField(std::string const& name, Object& field);
   
private:
   NO_COPY_CONSTRUCTOR(Record);
   friend ObjectDumper;

private:
   std::vector<std::pair<std::string, ObjectId>>   fields_;
   bool                    slave_;
   int                     registerOffset_;
   mutable int             saveCount_;
   bool                    constructed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_H