#pragma once
#include "object.h"
#include "store.pb.h"
#include "namespace.h"
#include "formatter.h"
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
   void CloneObject(Object* copy, CloneMapping& mapping) const override;
   std::ostream& Log(std::ostream& os, std::string indent) const override;

private:
   NO_COPY_CONSTRUCTOR(Record);

private:
   std::vector<std::pair<std::string, ObjectId>>   fields_;
   bool                    slave_;
   int                     registerOffset_;
   mutable int             saveCount_;
};

#if 0
template<>
struct SaveImpl<Record>
{
   static void SaveValue(const Store& store, Protocol::Value* msg, const Record& obj) {
      obj.SaveValue(store, msg);
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, Record& obj) {
      obj.LoadValue(store, msg);
   }
};
#endif

template<>
struct Formatter<Record>
{
   static std::ostream& Log(std::ostream& out, const Record& value, const std::string indent) {
      return value.Log(out, indent);
   }
};

END_RADIANT_DM_NAMESPACE

