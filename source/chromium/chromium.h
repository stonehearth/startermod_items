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

   typedef std::function<void(csg::Point2 const& size, csg::Region2 const & rgn, const uint32* buff)> PaintCb;
   typedef std::function<void(const HCURSOR cursor)> CursorChangeCb; // xxx: merge this into the command system!

   typedef std::function<void(std::string const& uri, JSONNode const& query, std::string const& postdata, rpc::HttpDeferredPtr response)> HandleRequestCb;

   virtual void Navigate(std::string const& uri) = 0;
   virtual bool HasMouseFocus(int screen_x, int screen_y) = 0;
   virtual void UpdateDisplay(PaintCb cb) = 0;
   virtual void Work() = 0;
   virtual bool OnInput(Input const& evt) = 0;
   virtual void SetCursorChangeCb(CursorChangeCb cb) = 0;
   virtual void SetRequestHandler(HandleRequestCb cb) = 0;
   virtual void OnScreenResize(csg::Point2 const& size) = 0;
};

IBrowser* CreateBrowser(HWND parentWindow, std::string const& docroot, csg::Point2 const& screenSize, csg::Point2 const& minUiSize, int debug_port);

END_RADIANT_CHROMIUM_NAMESPACE

#endif // _RADIANT_CHROMIUM_CHROMIUM_H
