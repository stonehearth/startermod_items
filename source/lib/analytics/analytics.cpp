#include "radiant.h"
#include "analytics.h"
#include "analytics_logger.h"
#include "design_event.h"

using namespace ::radiant;
using namespace ::radiant::analytics;

void analytics::StartSession(std::string const& userid, std::string const& sessionid, std::string const& build_number)
{
   //Initialize the logger for the first time
   analytics::AnalyticsLogger &a = analytics::AnalyticsLogger::GetInstance();
   a.SetBasicValues(userid, sessionid, build_number);

   analytics::DesignEvent("game:started").SendEvent();
}

void analytics::StopSession()
{
    analytics::DesignEvent("game:stopped").SendEvent();
}