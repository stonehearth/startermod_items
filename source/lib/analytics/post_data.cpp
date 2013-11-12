#include "radiant.h"
#include "radiant_logger.h"
#include "analytics_logger.h"
#include "post_data.h"
#include "lib/json/node.h"
#include <boost/algorithm/string.hpp>

using namespace ::radiant;
using namespace ::radiant::analytics;

//Helper data class, Event Data
PostData::PostData(json::Node node, std::string domain, std::string path, std::string authorization_string)
{
   node_ = node;
   domain_ = domain;
   path_ = path;
   authorization_string_ = authorization_string;
}

PostData::~PostData()
{
}

json::Node PostData::GetJsonNode()
{
   return node_;
}

std::string PostData::GetDomain()
{
   return domain_;
}

std::string PostData::GetPath()
{
   return path_;
}

std::string PostData::GetAuthorizationString()
{
   return authorization_string_;
}

void PostData::Send()
{
   analytics::AnalyticsLogger &logger = analytics::AnalyticsLogger::GetInstance();
   logger.SubmitPost(node_, domain_, path_, authorization_string_);
}