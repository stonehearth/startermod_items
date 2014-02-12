#ifndef _RADIANT_CHROMIUM_APP_APP_H
#define _RADIANT_CHROMIUM_APP_APP_H

#include "include/cef_app.h"
#include "chromium/chromium.h"

BEGIN_RADIANT_CHROMIUM_NAMESPACE

class App : public CefApp,      
            public CefBrowserProcessHandler,
            public CefRenderProcessHandler
{
public:
   IMPLEMENT_REFCOUNTING(App);

public: // CefApp overrides
};

END_RADIANT_CHROMIUM_NAMESPACE

#endif // _RADIANT_CHROMIUM_APP_APP_H
