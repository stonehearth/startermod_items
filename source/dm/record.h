#pragma once
#include "object.h"
#include "store.pb.h"
#include "namespace.h"
#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Record : public Object
{
public:
   DEFINE_DM_OBJECT_TYPE(Record);

   Record();

   void Initialize(Store& s, ObjectId id) override;
   void InitializeSlave(Store& s, ObjectId id) override;
   virtual void InitializeRecordFields() { };

   dm::Guard TraceRecordField(std::string name, const char* reason, std::function<void()> fn);  

   bool IsRemoteRecord() const { return slave_; }

protected:
   bool FieldIsUnique(std::string name, Object& field);
   void AddRecordField(std::string name, Object& field);
   void AddRecordField(Object& field);

public:
   void SaveValue(const Store& store, Protocol::Value* msg) const override;
   void LoadValue(const Store& store, const Protocol::Value& msg) override;

private:
   NO_COPY_CONSTRUCTOR(Record);

private:
   std::vector<std::pair<std::string, ObjectId>>   fields_;
   bool                    slave_;
   int                     registerOffset_;
   mutable int             saveCount_;
};

END_RADIANT_DM_NAMESPACE

