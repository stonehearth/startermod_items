#ifndef _RADIANT_OM_MODEL_VARIANTS_H
#define _RADIANT_OM_MODEL_VARIANTS_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class ModelVariant : public dm::Record
{
public:
   DEFINE_OM_OBJECT_TYPE(ModelVariant, model_variant);

public:
   enum Layer {
      SKELETON    = 0,
      SKIN        = 1,
      CLOTHING    = 2,
      ARMOR       = 3,
      CLOAK       = 4,
      NUM_LAYERS  = 5
   };

public:
   void ExtendObject(json::ConstJsonObject const& obj);

   dm::Set<std::string> const& GetModels() const { return models_; }
   void RemoveModel(std::string rig) { models_.Remove(rig); }
   Layer GetLayer() const { return (Layer)*layer_; }
   std::string GetVariants() const { return *variants_; }

private:
   NO_COPY_CONSTRUCTOR(ModelVariant)

   void InitializeRecordFields() override {
      dm::Record::InitializeRecordFields();
      AddRecordField("layer", layer_);
      AddRecordField("variants", variants_);
      AddRecordField("models", models_);
   }

private:
   dm::Boxed<int>          layer_;
   dm::Boxed<std::string>  variants_;
   dm::Set<std::string>    models_;
};

typedef std::shared_ptr<ModelVariant> ModelVariantPtr;
typedef std::weak_ptr<ModelVariant> ModelVariantRef;
std::ostream& operator<<(std::ostream& os, const ModelVariant& o);

class ModelVariants : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(ModelVariants, model_variants);
   void ExtendObject(json::ConstJsonObject const& obj) override;

public:
   ModelVariantPtr GetModelVariant(std::string const& v) const;

private:
   NO_COPY_CONSTRUCTOR(ModelVariants)

   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("variants", variants_);
   }

private:
   dm::Set<ModelVariantPtr>       variants_;

};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_MODEL_VARIANTS_H
