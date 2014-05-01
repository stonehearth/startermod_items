#include "radiant.h"
#include "radiant_logger.h"
#include "build_number.h"
#include "event_data.h"
#include "lib/json/node.h"
#include <boost/algorithm/string.hpp>

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "sha.h"
#include "hex.h"
#include "files.h"
#include "md5.h"


using namespace ::radiant;
using namespace ::radiant::analytics;

const std::string GAME_ANALYTICS_DOMAIN = "api.gameanalytics.com";
const std::string API_VERSION = "1";

//Helper data class, Event Data
EventData::EventData(const json::Node& event_node, std::string const& event_category)
: PostData(event_node, std::string(GAME_ANALYTICS_URI) + "/" + std::string(GAME_ANALYTICS_API_VERSION) + "/" + std::string(GAME_ANALYTICS_GAME_KEY) + "/" + event_category, "")
{
   event_node_ = event_node;
   event_category_ = event_category;

   std::string event_string = node_.write();

   // the header is the data + secret key
   std::string header = event_string + GAME_ANALYTICS_SECRET_KEY;

   // Hash the header with md5
   // Borrowing sample code from http://www.cryptopp.com/wiki/Hash_Functions
   CryptoPP::Weak::MD5 hash;
   byte digest[CryptoPP::Weak::MD5::DIGESTSIZE];
   hash.CalculateDigest(digest, (byte*)header.c_str(), header.length());

   CryptoPP::HexEncoder encoder;
   std::string digest_string;
   encoder.Attach(new CryptoPP::StringSink(digest_string));
   encoder.Put(digest, sizeof(digest));
   encoder.MessageEnd();

   // Super important to make sure the final hex is all lower case
   boost::algorithm::to_lower(digest_string);

   authorization_string_ = digest_string;
}

EventData::~EventData()
{
}

json::Node EventData::GetJsonNode()
{
   return event_node_;
}

std::string EventData::GetCategory()
{
   return event_category_;
}