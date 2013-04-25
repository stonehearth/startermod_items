#ifndef _RADIANT_CLIENT_ENTITY_TRACES_H
#define _RADIANT_CLIENT_ENTITY_TRACES_H

#include "namespace.h"
#include "libjson.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

typedef std::function<bool(om::EntityPtr)> EntityFilterFn;

class Writer;

class TraceInterface
{
public:
   TraceInterface(const JSONNode& trace);
   bool Flush(JSONNode& obj);
   
private:
   virtual void Write(JSONNode& obj) = 0;

protected:
   EntityFilterFn       filter_;
   JSONNode             collectorArgs_;
   dm::GuardSet         guards_;
   bool                 dirty_;
};

class EntitiesTrace : public TraceInterface
{
public:
   EntitiesTrace(const JSONNode& trace);

private:
   void TraceEntity(om::EntityPtr e);
   void Write(JSONNode& obj) override;

private:
   std::unordered_map<om::EntityId, std::vector<std::shared_ptr<Writer>>> collectors_;
};

class EntityTrace : public TraceInterface
{
public:
   EntityTrace(om::EntityPtr entity, const JSONNode& trace);

private:
   void Write(JSONNode& obj) override;

private:
   om::EntityId   entityId_;
   std::vector<std::shared_ptr<Writer>> collectors_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_ENTITY_TRACES_H
