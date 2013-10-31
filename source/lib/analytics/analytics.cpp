#include "radiant.h"
#include "analytics.h"
#include "analytics_logger.h"
#include "send_design_event.h"

using namespace ::radiant;
using namespace ::radiant::analytics;

void analytics::StartSession(std::string const& userid, std::string const& sessionid, std::string const& build_number, bool collect_analytics)
{
   //Initialize the logger for the first time
   analytics::AnalyticsLogger &a = analytics::AnalyticsLogger::GetInstance();
   a.SetBasicValues(userid, sessionid, build_number, collect_analytics);

   analytics::SendDesignEvent("game:started");
}

void analytics::StopSession()
{
    analytics::SendDesignEvent("game:stopped");
}