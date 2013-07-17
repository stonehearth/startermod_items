#ifndef _RADIANT_CHROMIUM_CHROMIUM_H
#define _RADIANT_CHROMIUM_CHROMIUM_H

#include "radiant_file.h" // xxx: rename to radiant_io or something?
#include "csg/region.h"
#include "namespace.h"

BEGIN_RADIANT_CHROMIUM_NAMESPACE


class IResponse
{
public:
   virtual ~IResponse() { }
   virtual void SetResponse(std::string response, std::string mimeType) = 0;
   virtual void SetStatusCode(int status) = 0;
};

class IBrowser
{
public:
   virtual ~IBrowser() {}

   typedef std::function<void(const csg::Region2& rgn, const char* buffer)> PaintCb;
   typedef std::function<void(const HCURSOR cursor)> CursorChangeCb; // xxx: merge this into the command system!

   typedef std::function<void(std::string const& uri, std::string const& query, std::string const& postdata, std::shared_ptr<IResponse> response)> HandleRequestCb;

   virtual bool HasMouseFocus() = 0;
   virtual void UpdateDisplay(PaintCb cb) = 0;
   virtual void Work() = 0;
   virtual void OnRawInput(const RawInputEvent &evt, bool& handled, bool& uninstall) = 0;
   virtual void OnMouseInput(const MouseEvent &evt, bool& handled, bool& uninstall) = 0;
   virtual void SetCursorChangeCb(CursorChangeCb cb) = 0;
   virtual void SetRequestHandler(HandleRequestCb cb) = 0;
};

IBrowser* CreateBrowser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port);

END_RADIANT_CHROMIUM_NAMESPACE

#endif // _RADIANT_CHROMIUM_CHROMIUM_H
