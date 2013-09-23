#ifndef _RADIANT_OM_ITEM_H
#define _RADIANT_OM_ITEM_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Item : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Item, item);
   void ExtendObject(json::ConstJsonObject const& obj);

public:
   //std::string GetKind() const { return *kind_; }
   //void SetKind(std::string kind) { kind_ = kind; }

   void SetStacks(int count) { stacks_ = count; }
   int GetStacks() { return *stacks_; }

   void SetMaxStacks(int count) { maxStacks_ = count; }
   int GetMaxStacks() { return *maxStacks_; }

   std::string GetMaterial() const { return *material_; }
   void SetMaterial(std::string cls) { material_ = cls; }

   std::string GetIdentifier() const {return *identifier_;}
   void SetIdentifier(std::string identifier) { identifier_ = identifier; }

   std::string GetCategory() const {return *category_;}
   void SetCategory(std::string category) { category_ = category; }


   dm::Guard TraceStacks(const char* reason, std::function<void()> fn) { return stacks_.TraceObjectChanges(reason, fn); };

private:
   NO_COPY_CONSTRUCTOR(Item)

   void InitializeRecordFields() override;

public:
   dm::Boxed<std::string>     kind_;
   dm::Boxed<int>             stacks_;
   dm::Boxed<int>             maxStacks_;
   dm::Boxed<std::string>     material_;
   dm::Boxed<std::string>     identifier_;
   dm::Boxed<std::string>     category_;

};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_ITEM_H
