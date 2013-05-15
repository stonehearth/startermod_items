#include "pch.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include "client.h"
#include "entity_traces.h"
#include "resources/res_manager.h"
#include "om/entity.h"
#define DEFINE_ALL_COMPONENTS
#include "om/all_components.h"

using namespace radiant;
using namespace radiant::client;

typedef std::vector<std::string>::iterator TokenizerIterator;

// Fetch the child node with the specified type if it exists.  If not,
// create it and return it.
JSONNode& FetchNodeChild(JSONNode& node, std::string name, int t = JSON_NODE)
{
   auto i = node.find(name);
   if (i == node.end()) {
      JSONNode child(t);
      child.set_name(name);
      node.push_back(child);
      i = node.find(name);
   }
   return *i;
}

EntityFilterFn GenerateFilterFn(const JSONNode& filter)
{
   EntityFilterFn cb, accum = [=](om::EntityPtr e) -> bool { return e != nullptr; };

   for (const auto& node : filter) {
      if (node.type() == JSON_STRING) {
         std::string query = node.as_string();
         if (query == "stockpile") {
            cb = [=](om::EntityPtr e) -> bool { return e->GetComponent<om::StockpileDesignation>() != nullptr; };
         } else if (query == "identity") {
            cb = [=](om::EntityPtr e) -> bool { return e->GetComponent<om::UnitInfo>() != nullptr; };
         } else if (query == "mob") {
            cb = [=](om::EntityPtr e) -> bool { return e->GetComponent<om::Mob>() != nullptr; };
         } else if (query == "profession") {
            cb = [=](om::EntityPtr e) -> bool { return e->GetComponent<om::Profession>() != nullptr; };
         } else {
            cb = [=](om::EntityPtr e) -> bool { return true; }; // ignore bogus requests...
         }
         // make sure we put the accumulator first to guard against null pointers as we
         // iterate down the filter string.
         accum = [=](om::EntityPtr e) -> bool { return accum(e) && cb(e); };
      }
   }
   return accum;
}

BEGIN_RADIANT_CLIENT_NAMESPACE

class Writer
{
public:
   virtual void Write(JSONNode& parent) = 0;
};

template <class OwnerType, class ObjectType>
class BasicWriter : public Writer
{
public:
   BasicWriter(const char* name, dm::Boxed<ObjectType>& obj, bool& changed) :
      name_(name),
      obj_(obj)
   {
      guard_ = obj.TraceObjectChanges("basic trace collector", [&changed] { changed = true; });
   }

   void Write(JSONNode& parent) override
   {
      parent.push_back(JSONNode(name_, *obj_));
   }

private:
   const char*                name_;
   dm::Guard                  guard_;
   dm::Boxed<ObjectType>&     obj_;
};

template <class ObjectType>
class GenericWriter : public Writer
{
public:
   typedef std::function<void(ObjectType& s, dm::GuardSet& guards, bool& changed)> InstallFn;
   typedef std::function<void(ObjectType& s, JSONNode& parent)> WriterFn;

   GenericWriter(ObjectType& obj, InstallFn install, WriterFn write, bool& changed) :
      obj_(obj),
      write_(write)
   {
      install(obj, guards_, changed);
   }

   void Write(JSONNode& parent) override
   {
      write_(obj_, parent);
   }

private:
   dm::GuardSet      guards_;
   ObjectType&       obj_;
   WriterFn          write_;
};


template <class OwnerType>
class ComponentWriter : public Writer
{
public:
   ComponentWriter(const char *name, std::shared_ptr<OwnerType> owner) :
      owner_(owner),
      name_(name)
   {
   }

   void Write(JSONNode& parent) override
   {
      auto owner = owner_.lock();
      if (owner) {
         JSONNode& node = FetchNodeChild(parent, name_);
         for (auto& c : writers_) {
            c->Write(node);
         }
      }
   }

protected:
   const char* name_;
   std::weak_ptr<OwnerType>   owner_;
   std::vector<std::shared_ptr<Writer>>   writers_;
};

class EntitySetWriter : public Writer
{
public:
   EntitySetWriter(const char* name, dm::Set<om::EntityId>& s, bool& changed, const JSONNode* writer);
   void Write(JSONNode& parent) override;

private:
   dm::Set<om::EntityId>& s_;
   dm::GuardSet   guards_;
   const char*    name_;
   const JSONNode* writer_;
   std::unordered_map<om::EntityId, std::vector<std::shared_ptr<Writer>>> writers_;
};


#define ADD_BASIC_COLLECTOR(Cls, Type, name, var) \
   if (begin == end || *begin == name) { \
      auto writer = new BasicWriter<Cls, Type>(name, var, changed); \
      writers_.push_back(std::shared_ptr<Writer>(writer)); \
   }

#define ADD_ENTITY_SET_COLLECTOR(Cls, name, var, subCollector) \
   if (begin == end || *begin == name) { \
      auto writer = new EntitySetWriter(name, var, changed, subCollector); \
      writers_.push_back(std::shared_ptr<Writer>(writer)); \
   }

#define ADD_GENERIC_COLLECTOR(Cls, name, obj, install, write) \
   if (begin == end || *begin == name) { \
      auto writer = new GenericWriter<Cls>(*obj, install, write, changed); \
      writers_.push_back(std::shared_ptr<Writer>(writer)); \
   }

class UnitInfoComponentWriter : public ComponentWriter<om::UnitInfo>
{
public:
   UnitInfoComponentWriter(om::UnitInfoPtr c, bool& changed, TokenizerIterator begin, TokenizerIterator end, JSONNode* subCollector) :
      ComponentWriter("identity", c)
   {
      ASSERT(c);
      ADD_BASIC_COLLECTOR(om::UnitInfo, std::string, "name", c->name_);
      ADD_BASIC_COLLECTOR(om::UnitInfo, std::string, "description", c->description_);
   }
};

class ItemComponentWriter : public ComponentWriter<om::Item>
{
public:
   ItemComponentWriter(om::ItemPtr c, bool& changed, TokenizerIterator begin, TokenizerIterator end, JSONNode* subCollector) :
      ComponentWriter("item", c)
   {
      ASSERT(c);
      ADD_BASIC_COLLECTOR(om::Item, int, "stacks", c->stacks_);
      ADD_BASIC_COLLECTOR(om::Item, int, "max_stacks", c->maxStacks_);
      ADD_BASIC_COLLECTOR(om::Item, std::string, "material", c->material_);
   }
};

class StockpileDesignationComponentWriter : public ComponentWriter<om::StockpileDesignation>
{
public:
   StockpileDesignationComponentWriter(om::StockpileDesignationPtr c, bool& changed, TokenizerIterator begin, TokenizerIterator end, JSONNode* subCollector) :
      ComponentWriter("stockpile", c)
   {
      ASSERT(c);
      ADD_ENTITY_SET_COLLECTOR(om::StockpileDesignation, "contents", c->contents_, subCollector);
      ADD_GENERIC_COLLECTOR(om::StockpileDesignation, "capacity", c,
         [](om::StockpileDesignation& s, dm::GuardSet& guards, bool& changed) {
            guards += s.TraceBounds("trace", [&changed]() { changed = true; });
         },
         [](om::StockpileDesignation& s, JSONNode& parent) {
            parent.push_back(JSONNode("capacity", s.GetBounds().area()));
         });
   }
};

class ActionListComponentWriter : public ComponentWriter<om::ActionList>
{
public:
   ActionListComponentWriter(om::ActionListPtr c, bool& changed, TokenizerIterator begin, TokenizerIterator end, JSONNode* subCollector) :
      ComponentWriter("action_list", c)
   {
      auto actionInstall = [](om::ActionList& c, dm::GuardSet& guards, bool& changed) {
         guards += c.actions_.TraceObjectChanges("trace", [&changed]() { changed = true; });
      };
      auto actionsWrite = [](om::ActionList& c, JSONNode& parent) {
         auto &resources = resources::ResourceManager2::GetInstance();
         JSONNode& node = FetchNodeChild(parent, "actions", JSON_ARRAY);
         for (auto actionName : c.GetActions()) {
            JSONNode entry;
            auto resource = resources.LookupResource(actionName);
            if (resource->GetType() == resources::Resource::JSON) {
               auto action = std::static_pointer_cast<resources::DataResource>(resource);
               node.push_back(action->GetJson());
            }
         }
      };
      ASSERT(c);
      ADD_GENERIC_COLLECTOR(om::ActionList, "actions", c, actionInstall, actionsWrite);
   }
};

class ProfessionComponentWriter : public ComponentWriter<om::Profession>
{
public:
   ProfessionComponentWriter(om::ProfessionPtr c, bool& changed, TokenizerIterator begin, TokenizerIterator end, JSONNode* subCollector) :
      ComponentWriter("profession", c)
   {
      auto actionInstall = [](om::Profession& c, dm::GuardSet& guards, bool& changed) {
         guards += c.learnedReceipes_.TraceObjectChanges("trace", [&changed]() { changed = true; });
      };
      auto actionsWrite = [](om::Profession& c, JSONNode& parent) {
         auto &resources = resources::ResourceManager2::GetInstance();
         JSONNode& node = FetchNodeChild(parent, "learned_recipes", JSON_ARRAY);
         for (auto recipe : c.GetLearnedRecipies()) {
            node.push_back(JSONNode("", recipe));
         }
      };
      ASSERT(c);
      ADD_GENERIC_COLLECTOR(om::Profession, "learned_recipes", c, actionInstall, actionsWrite);
   }
};

class EntityWriter : public Writer
{
public:
   EntityWriter(om::EntityPtr e, bool& changed) :
      entity_(e)
   {
      AddComponentCollector(e, changed, std::string(""), nullptr);
   }

   EntityWriter(om::EntityPtr e, bool& changed, const JSONNode& arg) :
      entity_(e)
   {
      changed = true;
      if (arg.type() == JSON_STRING) {
         AddComponentCollector(e, changed, arg.as_string(), nullptr);
      } else if (arg.type() == JSON_NODE) {
         std::string type = arg["type"].as_string();
         if (type == "entity_list") {
            subCollector_ = arg["collectors"];
            AddComponentCollector(e, changed, arg["property"].as_string(), &subCollector_);
         }
      }
   }

   void Write(JSONNode& parent) {
      auto entity = entity_.lock();
      if (entity) {
         JSONNode& node = FetchNodeChild(parent, stdutil::ToString(entity->GetEntityId()));
         for (auto writer : writers_) {
            writer->Write(node);
         }
      }
   }

private:
   void AddComponentCollector(om::EntityPtr e, bool& changed, std::string query, JSONNode* subCollector) {
      std::vector<std::string> tokens;
      boost::split(tokens, query, boost::is_any_of("."));
      auto begin = tokens.begin();
      auto end = tokens.end();

#define ADD_COMPONENT_COLLECTOR(Cls, name) \
      if (*begin == "" || *begin == (name)) { \
         auto component = e->GetComponent<om::Cls>(); \
         if (component) { \
            auto writer = std::shared_ptr<Writer>(new Cls ## ComponentWriter(component, changed, begin + 1, end, subCollector)); \
            writers_.push_back(writer); \
         } \
      }

      ADD_COMPONENT_COLLECTOR(UnitInfo,             "identity");
      ADD_COMPONENT_COLLECTOR(StockpileDesignation, "stockpile");
      ADD_COMPONENT_COLLECTOR(Item,                 "item");
      ADD_COMPONENT_COLLECTOR(ActionList,           "action_list");
      ADD_COMPONENT_COLLECTOR(Profession,           "profession");

#undef ADD_COMPONENT_COLLECTOR
   }

private:
   JSONNode subCollector_;
   om::EntityRef  entity_;
   std::vector<std::shared_ptr<Writer>> writers_;
};

EntitySetWriter::EntitySetWriter(const char* name, dm::Set<om::EntityId>& s, bool& changed, const JSONNode* writer) :
   writer_(writer),
   name_(name),
   s_(s)
{
   if (!writer) {
      guards_ += s.TraceObjectChanges("scc2", [&changed]() { changed = true; });
      return;
   }
   auto added = [this, &changed](om::EntityId id) {
      changed = true;
      auto entity = Client::GetInstance().GetEntity(id);
      if (entity) {
         writers_[id].clear();
         for (const auto& arg : *writer_) {
            auto c = std::make_shared<EntityWriter>(entity, changed, arg);
            writers_[id].push_back(c);
         }
      }
   };
   auto removed = [this, &changed](om::EntityId id) {
      changed = true;
      writers_[id].clear();
   };

   guards_ += s.TraceSetChanges("scc2", added, removed);
   for (const auto& id : s) {
      added(id);
   }
}

void EntitySetWriter::Write(JSONNode& parent) {
   if (!writer_) {
      JSONNode& contents = FetchNodeChild(parent, name_, JSON_ARRAY);
      for (const om::EntityId id : s_.GetContents()) {
         contents.push_back(JSONNode("", id));
      }
   } else {
      JSONNode& contents = FetchNodeChild(parent, name_);
      for (om::EntityId id : s_) {
         for (const auto& c : writers_[id]) {
            c->Write(contents);
         }
      }
   }
}

END_RADIANT_CLIENT_NAMESPACE

   TraceInterface::TraceInterface(const JSONNode& trace) :
   dirty_(true)
{
   filter_ = GenerateFilterFn(trace["filters"]);
   collectorArgs_ = trace["collectors"];
}


bool TraceInterface::Flush(JSONNode& obj)
{
   if (!dirty_) {
      return false;
   }
   Write(obj);
   dirty_ = false;
   return true;
}

EntitiesTrace::EntitiesTrace(const JSONNode& trace) :
   TraceInterface(trace)
{
   guards_ += Client::GetInstance().TraceDynamicObjectAlloc([=](dm::ObjectPtr obj) {
      if (obj->GetObjectType() == om::EntityObjectType) {
         TraceEntity(std::static_pointer_cast<om::Entity>(obj));
      }
   });
   Client::GetInstance().EnumEntities([=](om::EntityPtr e) {
      TraceEntity(e);
   });
}

void EntitiesTrace::TraceEntity(om::EntityPtr e)
{
   if (filter_(e)) {
      auto id = e->GetEntityId();
      collectors_[id].clear();
      if (collectorArgs_.empty()) {
         collectors_[id].push_back(std::make_shared<EntityWriter>(e, dirty_));
      } else {
         for (const auto& arg : collectorArgs_) {
            collectors_[id].push_back(std::make_shared<EntityWriter>(e, dirty_, arg));
         }
      }
   }
}

void EntitiesTrace::Write(JSONNode& obj)
{
   obj = JSONNode(JSON_NODE);
   for (const auto& entry : collectors_) {
      for (const auto& collector : entry.second) {
         collector->Write(obj);
      }
   }
}

EntityTrace::EntityTrace(om::EntityPtr e, const JSONNode& trace) :
   TraceInterface(trace),
   entityId_(0)
{
   if (filter_(e)) {
      entityId_ = e->GetEntityId();
      if (collectorArgs_.empty()) {
         collectors_.push_back(std::make_shared<EntityWriter>(e, dirty_));
      } else {
         for (const auto& arg : collectorArgs_) {
            collectors_.push_back(std::make_shared<EntityWriter>(e, dirty_, arg));
         }
      }
   }
}

void EntityTrace::Write(JSONNode& obj)
{
   if (entityId_) {
      JSONNode container;
      for (const auto& c : collectors_) {
         c->Write(container);
      }
      obj = FetchNodeChild(container, stdutil::ToString(entityId_));
   }
}
