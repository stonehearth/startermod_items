#ifndef _RADIANT_DM_DM_LOG_H
#define _RADIANT_DM_DM_LOG_H

#define DM_LOG(category, level) \
   LOG(INFO) << "data model | " << category << " | "

#undef DM_LOG
#define DM_LOG(c, l) if (false) LOG(INFO)

#define TRACE_LOG(level)  DM_LOG("trace", level)

#endif //  _RADIANT_DM_DM_LOG_H
