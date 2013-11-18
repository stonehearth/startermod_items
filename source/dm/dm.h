#ifndef _RADIANT_DM_NAMESPACE_H
#define _RADIANT_DM_NAMESPACE_H

#define BEGIN_RADIANT_DM_NAMESPACE  namespace radiant { namespace dm {
#define END_RADIANT_DM_NAMESPACE    } }

// xxx: move this into a protocol.h header?
namespace Protocol {
   class Value;
};

BEGIN_RADIANT_DM_NAMESPACE
 
typedef int GenerationId;
typedef int TraceId;
typedef int ObjectId;
typedef int ObjectType;

class Object;
class ObjectDumper;
class Store;

class Trace;
class TraceSync;
class TraceBuffered;
class Tracer;
class TracerSync;
class TracerBuffered;
class Stream;
class Streamer;
class Receiver;

class Record;
template <typename T> class ObjectTrace;
template <typename T> class RecordTrace;

DECLARE_SHARED_POINTER_TYPES(Object)
DECLARE_SHARED_POINTER_TYPES(Store)
DECLARE_SHARED_POINTER_TYPES(Trace)
DECLARE_SHARED_POINTER_TYPES(TraceSync)
DECLARE_SHARED_POINTER_TYPES(TraceBuffered)
DECLARE_SHARED_POINTER_TYPES(Tracer)
DECLARE_SHARED_POINTER_TYPES(TracerSync)
DECLARE_SHARED_POINTER_TYPES(TracerBuffered)
DECLARE_SHARED_POINTER_TYPES(Streamer)
DECLARE_SHARED_POINTER_TYPES(Receiver)

void LoadObject(Record& record, Protocol::Value const& msg);
void SaveObject(Record const& store, Protocol::Value* msg);
void SaveObjectDelta(Record const& store, Protocol::Value* msg);

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_NAMESPACE_H
