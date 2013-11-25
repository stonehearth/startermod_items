#ifndef _RADIANT_JSON_JSON_H
#define _RADIANT_JSON_JSON_H

#include <iostream>
#include <libjson.h>
#include "namespace.h"

BEGIN_RADIANT_JSON_NAMESPACE

void InitialzeErrorHandler();

bool ReadJsonFile(std::string const& filename, JSONNode& dst_node, std::string& error_message);
bool ReadJson(std::istream& stream, JSONNode& dst_node, std::string& error_message);
bool ReadJson(json_string const& text, JSONNode& dst_node, std::string& error_message);

END_RADIANT_JSON_NAMESPACE

#endif // _RADIANT_JSON_JSON_H
