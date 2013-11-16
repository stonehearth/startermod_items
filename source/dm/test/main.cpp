// datamover.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boxed.h"
#include "record.h"
#include "store.h"
#include "set.h"
#include "map.h"
#include "test_record.h"

class SourceBridge
{
public:
   SourceBridge(Store& source, Store& sink) :
      src_(source), dst_(sink)
   {
   }

   void Sync()
   {
      src_.SaveModifiedSince(0, [&](Protocol::Object& obj) {
         dst_.LoadObject(obj);
      });
   }

protected:
   Store&   src_;
   Store&   dst_;
};


template<class T> void TestSet(T v0, T v1, T v2)
{
   Store source(1), sink(2);
   SourceBridge b(source, sink);

   Set<T> s, t;

   s.Initialize(source, 100);
   t.Initialize(sink, 100);

   s.Insert(v0);
   b.Sync();
   ASSERT(s.GetContents() == t.GetContents());
   for (const T& item : s) {
      ASSERT(item == v0);
   }

   s.Insert(v1);
   s.Insert(v2);
   b.Sync();
   ASSERT(s.GetContents() == t.GetContents());
}

void TestMap()
{
   Store source(1), sink(2);
   SourceBridge b(source, sink);

   Map<int, int> s, t;

   s.Initialize(source, 100);
   t.Initialize(sink, 100);

   s[13] = 14;
   b.Sync();
   ASSERT(s.GetContents() == t.GetContents());

   for (const auto& entry : s) {
      ASSERT(entry.first == 13);
      ASSERT(entry.second == 14);
   }
   for (const auto& entry : t) {
      ASSERT(entry.first == 13);
      ASSERT(entry.second == 14);
   }
}

void TestObjectRemoting()
{
   Store source(1), sink(2);
   {
      SourceBridge b(source, sink);
      std::shared_ptr<RecordTest> s = source.AllocObject<RecordTest>();
      s->ivalue0 = 1;
      s->ivalue1 = 2;

      b.Sync();

      std::shared_ptr<RecordTest> d = sink.FetchObject<RecordTest>(s->GetObjectId());

      ASSERT(d->ivalue0 == s->ivalue0);
      ASSERT(d->ivalue1 == s->ivalue1);
   }
}

template <class T> void TestObjectPersistance(Store &source, Store &sink, typename T::Value value)
{
   T::Value initializer;
   Protocol::Object msg;

   T t(source);
   t = value;
   t.SaveObject(&msg);
   t = initializer;
   ASSERT(t == initializer);

   t.LoadObject(msg);
   ASSERT(t == value);
}

template <class T> void TestObjectTrace(typename T::Value v1, typename T::Value v2)
{
   Store source(1), sink(2);

   int c = 0;
   auto fn = [&]() {
      c++;
   };

   T obj(source);
   obj = v1;
   source.FireTraces();
   ASSERT(c == 0);

   {
      core::Guard trace = obj.TraceObjectChanges(fn);
      
      source.FireTraces();
      ASSERT(c == 0);
   
      obj = v2;
      source.FireTraces();
      ASSERT(c == 1);
   }
   obj = v1;
   source.FireTraces();
   ASSERT(c == 1);
}

template <class T> void TestObjectAssignment(Store &source, Store &sink, typename T::Value value)
{
   T::Value initializer;
   T t(source);

   // assignment...
   t = initializer;
   ASSERT(t == initializer);

#if 0
   t = value;
   ASSERT(t == value);

   // checkout type assignment
   t = initializer;
   T::Value& v = t.Modify();
   ASSERT(v == initializer);
   ASSERT(t == initializer);
   v = value;
   ASSERT(t == value);
#endif
}

template <class T> void TestObjectLifetime(Store &source, Store &sink)
{
   int c;
   auto dtor = [&c]() {
      c++;
   };

   // Ensure the trace function works.
   c = 0;
   {
      core::Guard trace;
      {
         // std::cout << "enter core::Guard scope..." << std::endl;
         T t(source);
         ASSERT(source.GetObjectCount() == 1 && source.GetTraceCount() == 0);

         // std::cout << "installing dtor..." << std::endl;
         trace = t.TraceObjectLifetime(dtor);
         ASSERT(source.GetObjectCount() == 1 && source.GetTraceCount() == 1);
         ASSERT(c == 0);

         // std::cout << "leaving core::Guard scope..." << std::endl;
      }
      ASSERT(source.GetObjectCount() == 0 && source.GetTraceCount() == 0);
      ASSERT(c == 1);
   }
   ASSERT(c == 1);
   ASSERT(source.GetObjectCount() == 0 && source.GetTraceCount() == 0);

   // Ensure the trace funciton does not get called when core::Guard is destroyed
   c = 0;
   {
      T(source).TraceObjectLifetime(dtor);
   }
   ASSERT(c == 0);
   ASSERT(source.GetObjectCount() == 0 && source.GetTraceCount() == 0);

   // Ensure the core::Guard can be moved
   c = 0;
   {
      core::Guard t1, t2;
      {
         T t(source);
         t1 = t.TraceObjectLifetime(dtor);
         std::swap(t1, t2);
      }
   }
   ASSERT(c == 1);
   ASSERT(source.GetObjectCount() == 0 && source.GetTraceCount() == 0);

   // Ensure multiple traces work
   c = 0;
   {
      core::Guard t1, t2, t3;
      {
         T t(source);
         t1 = t.TraceObjectLifetime(dtor);
         t2 = t.TraceObjectLifetime(dtor);
         t3 = t.TraceObjectLifetime(dtor);
      }
   }
   ASSERT(c == 3);
   ASSERT(source.GetObjectCount() == 0 && source.GetTraceCount() == 0);

   // Ensure we can unregister a trace without screwing up others
   // std::cout << "---------------" << std::endl;
   c = 0;
   {
      core::Guard t1;
      {
         T t(source);
         auto t2 = t.TraceObjectLifetime(dtor);
         t1 = t.TraceObjectLifetime(dtor);
      }
   }
   ASSERT(source.GetObjectCount() == 0 && source.GetTraceCount() == 0);
}

template <class T> void TestType()
{
   Store source(1), sink(2);

   TestObjectLifetime<T>(source, sink);
   TestObjectAssignment<T>(source, sink, 5);
   TestObjectPersistance<T>(source, sink, 5);
   TestObjectTrace<T>(5, 4);
}

int _tmain(int argc, _TCHAR* argv[])
{
   TestType<Boxed<int>>();
   TestSet<int>(13, 42, 1972);
   TestMap();
   TestObjectRemoting();

	return 0;
}
