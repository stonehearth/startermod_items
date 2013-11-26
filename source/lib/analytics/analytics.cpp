#include "radiant.h"
#include "analytics.h"
#include "analytics_logger.h"
#include "design_event.h"
#include "core/config.h"

using namespace ::radiant;
using namespace ::radiant::analytics;

static std::string const collect_analytics_property_name = "collect_analytics";

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

bool analytics::IsCollectionStatusSet()
{
   return core::Config::GetInstance().Has(collect_analytics_property_name);
}

bool analytics::GetCollectionStatus()
{
   return core::Config::GetInstance().Get(collect_analytics_property_name, false);
}

void analytics::SetCollectionStatus(bool should_collect) 
{
   core::Config::GetInstance().Set(collect_analytics_property_name, should_collect);
}
