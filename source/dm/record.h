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
   void LoadValue(Protocol::Value const& msg) override;
   void SaveValue(Protocol::Value* msg) const override;

   void Initialize(Store& s, ObjectId id) override;
   void InitializeSlave(Store& s, ObjectId id) override;

   virtual void ConstructObject() { };
   virtual void InitializeRecordFields() { };

   bool IsRemoteRecord() const { return slave_; }
   std::vector<std::pair<std::string, ObjectId>> const& GetFields() const { return fields_; }

protected:
   friend void dm::LoadObject(Record& record, Protocol::Value const& msg);

   bool FieldIsUnique(std::string name, Object& field);
   void AddRecordField(std::string name, Object& field);
   void AddRecordField(std::string name, ObjectId id);
   
private:
   NO_COPY_CONSTRUCTOR(Record);
   friend ObjectDumper;

private:
   std::vector<std::pair<std::string, ObjectId>>   fields_;
   bool                    slave_;
   int                     registerOffset_;
   mutable int             saveCount_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_RECORD_H