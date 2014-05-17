
template <class Object> Object* Simulation::CreateNewObject()
{
   Object *obj = new Object(this);
   _allObjects[obj->GetId()] = obj;
   return obj;
}

template <class Object> Object* Simulation::CreateNewObjectFromPrototype(string prototypeName)
{
   typename Object::Prototype *prototype = rootEntity_->GetPrototypeObject<typename Object::Prototype>(prototypeName);
   if (!prototype) {
      const JSONNode json = GetPrototypeTemplate(prototypeName);
      prototype = new typename Object::Prototype(json, this);
      rootObject->CachePrototypeObject(prototypeName, prototype);
      _allObjects[prototype->GetId()] = prototype;
   }
   auto obj = new Object(this, prototype);
   _allObjects[obj->GetId()] = obj;
   return obj;
}

template <class Object> Object* Simulation::CreateNewObjectFromRecipe(ObjectModel::IRecipe *recipe, ObjectModel::IngredientMap ingredients)
{
   ASSERT(false); // xxx: HA!
   return NULL;
}
