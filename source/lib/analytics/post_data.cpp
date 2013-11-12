#include "radiant.h"
#include "radiant_logger.h"
#include "analytics_logger.h"
#include "post_data.h"
#include "lib/json/node.h"
#include <boost/algorithm/string.hpp>

using namespace ::radiant;
using namespace ::radiant::analytics;

//Helper data class, Event Data
PostData::PostData(json::Node node, std::string uri, std::string authorization_string)
{
   node_ = node;
   uri_ = uri;
   authorization_string_ = authorization_string;
}

PostData::~PostData()
{
}

json::Node PostData::GetJsonNode()
{
   return node_;
}

std::string PostData::GetUri()
{
   return uri_;
}

std::string PostData::GetAuthorizationString()
{
   return authorization_string_;
}

void PostData::Send()
{
   analytics::AnalyticsLogger &logger = analytics::AnalyticsLogger::GetInstance();
   logger.SubmitPost(node_, uri_, authorization_string_);
}