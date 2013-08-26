#ifndef _RADIANT_OM_TARGET_TABLES_H
#define _RADIANT_OM_TARGET_TABLES_H

#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "dm/map.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

// --------------------------------------------------------------------------------------

class TargetTableEntry : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(TargetTableEntry, target_table_entry);

   void SetTarget(EntityPtr entity) { entity_ = entity; }

   EntityRef GetTarget() const { return *entity_; }   
   int GetValue() const { return *value_; }
   int GetExpireTime() const { return *expires_; }

   TargetTableEntry& SetValue(int value) { value_ = value; return *this; }   
   void SetExpireTime(int expires) { expires_ = expires; }

   bool Update(int now, int interval);

private:
   void InitializeRecordFields() override {
      AddRecordField("entity",   entity_);
      AddRecordField("value",    value_);
      AddRecordField("expires",  expires_);
      expires_ = 0;
      value_ = 0;
   }

private:
   dm::Boxed<EntityRef>       entity_;
   dm::Boxed<int>             value_;
   dm::Boxed<int>             expires_;
};
std::ostream& operator<<(std::ostream& os, const TargetTableEntry& o);
typedef std::shared_ptr<TargetTableEntry> TargetTableEntryPtr;

// --------------------------------------------------------------------------------------

class TargetTable : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(TargetTable, target_table);

   TargetTableEntryPtr GetEntry(EntityId id) { return entries_.Lookup(id, nullptr); }
   TargetTableEntryPtr AddEntry(EntityRef e);

   std::string GetName() const { return *name_; }
   void SetName(std::string name) { name_ = name; }

   std::string GetCategory() const { return *category_; }
   void SetCategory(std::string category) { category_ = category; }

   void Update(int now, int interval);
   const dm::Map<EntityId, TargetTableEntryPtr>& GetEntries() const { return entries_; }

private:
   void InitializeRecordFields() override {
      AddRecordField("entries",     entries_);
      AddRecordField("name",        name_);
      AddRecordField("category",    category_);
   }

private:
   dm::Boxed<std::string>                       name_;
   dm::Boxed<std::string>                       category_;
   dm::Map<EntityId, TargetTableEntryPtr> entries_;
};
std::ostream& operator<<(std::ostream& os, const TargetTable& o);
typedef std::shared_ptr<TargetTable> TargetTablePtr;

// --------------------------------------------------------------------------------------

struct TargetTableTop {
   TargetTableTop() { }

   EntityRef      target;
   int            value;
   int            expires;
};
std::ostream& operator<<(std::ostream& os, const TargetTableTop& o);
typedef std::shared_ptr<TargetTableTop> TargetTableTopPtr;

// --------------------------------------------------------------------------------------

class TargetTableGroup : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(TargetTableGroup, target_table_group);

   TargetTablePtr AddTable();
   void RemoveTable(TargetTablePtr table);
   TargetTableTopPtr GetTop();

   void SetCategory(std::string category) { category_ = category; }
   std::string GetCategory() const { return *category_; }

   int GetTableCount() const { return tables_.Size(); }
   void Update(int now, int interval);

private:
   void InitializeRecordFields() override {
      AddRecordField("tables",   tables_);
      AddRecordField("category", category_);
   }

private:
   dm::Boxed<std::string>     category_;
   dm::Set<TargetTablePtr>    tables_;
};
std::ostream& operator<<(std::ostream& os, const TargetTableGroup& o);
typedef std::shared_ptr<TargetTableGroup> TargetTableGroupPtr;

// --------------------------------------------------------------------------------------

class TargetTables : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(TargetTables, target_tables);

   TargetTablePtr AddTable(std::string category);
   void RemoveTable(TargetTablePtr table);

   TargetTableTopPtr GetTop(std::string category);

   void Update(int now, int interval);

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("tables", tables_);
   }

public:
   dm::Map<std::string, TargetTableGroupPtr>   tables_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_TARGET_TABLES_H
