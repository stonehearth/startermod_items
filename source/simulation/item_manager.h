#ifndef _RADIANT_SIMULATION_ITEM_MANAGER_H
#define _RADIANT_SIMULATION_ITEM_MANAGER_H

#include "world.h"
#include "objectModel.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Simulation;

class ItemManager : public ObjectModel::ObjectFactoryImpl {
public:
   ItemManager(Simulation *sim);

   std::shared_ptr<world::Object> CreateObject(world::DmType type) override
   {
      return _sim->GetWorld()->CreateObject(type);
   }

   std::shared_ptr<world::Object> GetCachedPrototype(std::string name) override
   {
      auto i = _prototypes.find(name);
      return i == _prototypes.end() ? NULL : i->second;
   }

   // ----

   typedef map<string, ObjectModel::Prototype> Inputs;

   template <class T> typename T::Prototype GetPrototype(std::string name) {
      T::Prototype prototype = LookupPrototype<T>(name);
      if (prototype.IsValid()) {
         return prototype;
      }
      return CreateSimplePrototype<T>(name);
   }

   template <class T> T CreateInstance(std::string prototypeName) {
      T::Prototype prototype = GetPrototype<T>(prototypeName);
      return T(_sim, prototype);
   }

   template <class T> T CreateFromRecipe(std::string recipe, const Inputs &ingredients) {
      std::string id = CreateRecipeId(recipe, ingredients);

      T::Prototype prototype = LookupPrototype<T>(id);
      if (!prototype.IsValid()) {
         prototype = CreateRecipePrototype<T>(id, recipe, ingredients);
      }
      return T(_sim, prototype);
   }

   std::shared_ptr<world::List> ToStringList(const JSONNode &obj)
   {
      std::shared_ptr<world::List> result = CreateObject<world::List>();
      for (auto i = obj.begin(); i != obj.end(); i++) {
         result->Add((*i).asString());
      }
      return result;
   }

protected:
   template <class T> typename T::Prototype LookupPrototype(std::string id)
   {
      auto i = _prototypes.find(id);
      if (i != _prototypes.end()) {
         return T::Prototype(_sim, i->second.GetObject());
      }
      return T::Prototype();
   }

   template <class T> typename T::Prototype CreateSimplePrototype(std::string id)
   {
      resources::Pattern *pattern = _sim->GetPatternManager().Lookup(id);
      return CreatePrototypeFromPattern<T>(id, pattern);
   }

   template <class T> typename T::Prototype CreateRecipePrototype(std::string id, std::string recipe, const Inputs &ingredients)
   {    
      JSONNode inputs;
      for_each(ingredients.begin(), ingredients.end(), [&](const Inputs::value_type &input) {
         inputs[input.first] = input.second.GetJson();
      });
      resources::Pattern *pattern = _sim->GetPatternManager().Generate(recipe, inputs);
      return CreatePrototypeFromPattern<T>(id, pattern);
   }

   template <class T> typename T::Prototype CreatePrototypeFromPattern(std::string id, resources::Pattern *pattern)
   {
      ASSERT(_prototypes.find(id) == _prototypes.end());

      T::Prototype prototype(_sim, _sim->GetWorld()->CreateObject<world::Container>());
      prototype.Initialize(pattern->GetJson());
      _prototypes[id] = prototype;
      return prototype;
   }


   std::string CreateRecipeId(std::string id, const Inputs &ingredients);

private:
   Simulation              *_sim;
   map<string, Prototype>  _prototypes;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_ITEM_MANAGER_H
