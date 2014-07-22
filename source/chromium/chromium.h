#ifndef _RADIANT_CHROMIUM_CHROMIUM_H
#define _RADIANT_CHROMIUM_CHROMIUM_H

#include <functional>
#include "radiant_net.h"
#include "csg/region.h"
#include "core/input.h"
#include "lib/rpc/forward_defines.h"

#define BEGIN_RADIANT_CHROMIUM_NAMESPACE  namespace radiant { namespace chromium {
#define END_RADIANT_CHROMIUM_NAMESPACE    } }

BEGIN_RADIANT_CHROMIUM_NAMESPACE

class IBrowser
{
public:
   virtual ~IBrowser() {}

   typedef std::function<void(const csg::Region2& rgn, const uint32* buff)> PaintCb;
   typedef std::function<void(const HCURSOR cursor)> CursorChangeCb; // xxx: merge this into the command system!

   typedef std::function<void(std::string const& uri, JSONNode const& query, std::string const& postdata, rpc::HttpDeferredPtr response)> HandleRequestCb;

   virtual void Navigate(std::string const& uri) = 0;
   virtual bool HasMouseFocus(int screen_x, int screen_y) = 0;
   virtual void UpdateDisplay(PaintCb cb) = 0;
   virtual void Work() = 0;
   virtual bool OnInput(Input const& evt) = 0;
   virtual void SetCursorChangeCb(CursorChangeCb cb) = 0;
   virtual void SetRequestHandler(HandleRequestCb cb) = 0;
   virtual void OnScreenResize(int w, int h) = 0;
   virtual void GetBrowserSize(int& w, int& h) = 0;
};

IBrowser* CreateBrowser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port);

END_RADIANT_CHROMIUM_NAMESPACE

#endif // _RADIANT_CHROMIUM_CHROMIUM_H
