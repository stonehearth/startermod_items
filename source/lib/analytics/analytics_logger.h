#ifndef _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
#define _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H

#include "analytics.h"
#include "core/singleton.h"
#include "lib/json/node.h"

BEGIN_RADIANT_ANALYTICS_NAMESPACE

class AnalyticsLogger : public core::Singleton<AnalyticsLogger>
{
public:
   AnalyticsLogger();
   ~AnalyticsLogger();

   void SetBasicValues(std::string userid, std::string sessionid, std::string build_version);
   void SubmitLogEvent(json::Node, std::string);

private:
    std::string userid_;
    std::string sessionid_;
    std::string build_version_;
};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
