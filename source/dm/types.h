#pragma once
#include <cassert>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <cstdio>
#include <memory>
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

struct ObjectIdentifier {
#if 1
   unsigned int      id;
   unsigned int      store;
#else
   unsigned int      id:30;
   unsigned int      store:2;
#endif
};

typedef int GenerationId;
typedef int TraceId;
typedef int ObjectId;
typedef int ObjectType;

class Object;

struct CloneMapping {
   std::unordered_map<ObjectId, ObjectId> objects;
   std::unordered_map<ObjectId, std::shared_ptr<Object> > dynamicObjects;
   std::unordered_map<ObjectId, std::shared_ptr<Object> > dynamicObjectsInverse;
   std::function<bool(std::shared_ptr<Object>, std::shared_ptr<Object>)> filter;

   std::vector<std::function<void()>> restore;
};

END_RADIANT_DM_NAMESPACE
