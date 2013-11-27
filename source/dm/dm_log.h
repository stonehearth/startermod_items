#ifndef _RADIANT_DM_DM_LOG_H
#define _RADIANT_DM_DM_LOG_H

#define DM_LOG(category, level) \
   LOG(INFO) << "data model | " << category << " | "

#define TRACE_LOG(level)  DM_LOG("trace", level)

#endif //  _RADIANT_DM_DM_LOG_H
