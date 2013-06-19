#ifndef _RADIANT_OM_PROFESSION_H
#define _RADIANT_OM_PROFESSION_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/set.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Profession : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Profession);

   const dm::Set<std::string>& GetLearnedRecipies() const { return learnedReceipes_; }

   void LearnReceipe(std::string value) { learnedReceipes_.Insert(value); }

private:
   void InitializeRecordFields() override;

public:
   dm::Set<std::string>    learnedReceipes_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_PROFESSION_H
