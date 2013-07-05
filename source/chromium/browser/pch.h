#ifndef _RADIANT_CLIENT_CHROMIUM_PCH_H
#define _RADIANT_CLIENT_CHROMIUM_PCH_H

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <unordered_map>
#include <mutex>

#include "radiant.h"

// CefDOMNode has methods GetNextSibling and GetFirstChild which
// conflict with their definitions in windows.h.  
#if defined(GetNextSibling)
#  undef GetNextSibling
#  undef GetFirstChild
#endif

#include "include/cef_url.h"
#include "include/cef_resource_handler.h"
#include "include/cef_origin_whitelist.h"
#include "include/cef_app.h"
#include "include/cef_base.h"
#include "include/cef_client.h"
#include "include/cef_browser.h"
#include "include/cef_resource_handler.h"
#include "libjson.h"

#endif // _RADIANT_CLIENT_CHROMIUM_PCH_H