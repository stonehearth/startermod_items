#ifndef _RADIANT_PERFMON_NAMESPACE_H
#define _RADIANT_PERFMON_NAMESPACE_H

#include <cstdint>
#include "radiant_macros.h"

#define BEGIN_RADIANT_PERFMON_NAMESPACE  namespace radiant { namespace perfmon {
#define END_RADIANT_PERFMON_NAMESPACE    } }

BEGIN_RADIANT_PERFMON_NAMESPACE

// Types
typedef int64_t CounterValueType;

// Forward Defines
class Timer;
class Counter;
class Frame;
class Timeline;
class Store;
class Hud;

END_RADIANT_PERFMON_NAMESPACE

#endif //  _RADIANT_PERFMON_NAMESPACE_H
