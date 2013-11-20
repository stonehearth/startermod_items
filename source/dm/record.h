#ifndef _RADIANT_DM_RECORD_H
#define _RADIANT_DM_RECORD_H

#include "object.h"
#include "protocols/store.pb.h"

BEGIN_RADIANT_DM_NAMESPACE

class Record : public Object
{
public:
   DEFINE_DM_OBJECT_TYPE(Record, record);
   DECLARE_STATIC_DISPATCH(Record);

   Record();

   void GetDbgInfo(DbgInfo &info) const override;

   void Initialize(Store& s, ObjectId id) override;
   void InitializeSlave(Store& s, ObjectId id) override;
   virtual void ConstructObject() { };
   virtual void InitializeRecordFields() { };

   bool IsRemoteRecord() const { return slave_; }
   std::vector<std::pair<std::string, ObjectId>> const& GetFields() const;

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