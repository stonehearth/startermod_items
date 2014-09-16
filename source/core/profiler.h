#ifndef _RADIANT_CORE_PROFILER_H
#define _RADIANT_CORE_PROFILER_H

#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

bool IsProfilerAvailable();
bool IsProfilerEnabled();
void SetProfilerEnabled(bool value);

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_PROFILER_H
