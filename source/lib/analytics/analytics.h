#ifndef _RADIANT_ANALYTICS_ANALYTICS_H
#define _RADIANT_ANALYTICS_ANALYTICS_H

#define BEGIN_RADIANT_ANALYTICS_NAMESPACE  namespace radiant { namespace analytics {
#define END_RADIANT_ANALYTICS_NAMESPACE    } }

BEGIN_RADIANT_ANALYTICS_NAMESPACE

class SendDesignEvent;

void StartSession(std::string const& userid, std::string const& sessionid, std::string const& build_number, bool collect_analytics);
void StopSession();

END_RADIANT_ANALYTICS_NAMESPACE

#endif //  _RADIANT_ANALYTICS_ANALYTICS_H
