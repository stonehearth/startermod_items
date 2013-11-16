#ifndef _RADIANT_DM_NAMESPACE_H
#define _RADIANT_DM_NAMESPACE_H

#define BEGIN_RADIANT_DM_NAMESPACE  namespace radiant { namespace dm {
#define END_RADIANT_DM_NAMESPACE    } }

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

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_NAMESPACE_H
