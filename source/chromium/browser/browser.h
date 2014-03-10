#ifndef _RADIANT_CHROMIUM_BROWSER_BROWSER_H
#define _RADIANT_CHROMIUM_BROWSER_BROWSER_H

#include "csg/region.h"
#include "../chromium.h"

BEGIN_RADIANT_CHROMIUM_NAMESPACE

class Browser : public IBrowser,
                public CefClient,
                public CefLifeSpanHandler,
                public CefSchemeHandlerFactory,
                public CefLoadHandler,
                public CefDisplayHandler,
                public CefRenderHandler,
                public CefKeyboardHandler,
                public CefRequestHandler
{
public:
   Browser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port);
   virtual ~Browser();

   typedef std::function<void(const csg::Region2& rgn)> PaintCb;
   typedef std::function<void(const CefCursorHandle cursor)> CursorChangeCb;
   
public:  // IBrowser Interface
   void Navigate(std::string const& uri) override;
   bool HasMouseFocus(int screen_x, int screen_y) override;
   void UpdateDisplay(PaintCb cb) override;
   void Work() override;
   bool OnInput(Input const& evt) override;
   void SetCursorChangeCb(CursorChangeCb cb) override;
   void SetRequestHandler(HandleRequestCb cb) override;
   void OnScreenResize(int w, int h) override;
   void GetBrowserSize(int& w, int& h) override;
   void UpdateBrowserFrambufferPtrs(uint32* last_written, uint32* next_to_write);

public:
   typedef int CommandId;

private:
   void OnRawInput(const RawInput& evt);
   void OnMouseInput(const MouseInput &evt);

public:
   IMPLEMENT_REFCOUNTING(Browser);

   // CefClient overrides
   CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
   CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
   CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
   CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
   CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

   // CefLifeSpanHandler overrides
   bool OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
      const CefPopupFeatures& popupFeatures,
      CefWindowInfo& windowInfo,
      const CefString& url,
      CefRefPtr<CefClient>& client,
      CefBrowserSettings& settings);
   void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
   bool RunModal(CefRefPtr<CefBrowser> browser) override;
   bool DoClose(CefRefPtr<CefBrowser> browser) override;
   void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
   void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) override;
   void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;

   // CefRequestHandler methods
   CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser,
                                                            CefRefPtr<CefFrame> frame,
                                                            CefRefPtr<CefRequest> request) override;
   bool OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                               const CefString& origin_url,
                               int64 new_size,
                               CefRefPtr<CefQuotaCallback> callback) override { return false; }
   void OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                    const CefString& url,
                                    bool& allow_os_execution) override;


   // CefDisplayHandler methods
  void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                    bool isLoading,
                                    bool canGoBack,
                                    bool canGoForward) override {}
  void OnAddressChange(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString& url) override;
  void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString& title) override;
  bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                const CefString& message,
                                const CefString& source,
                                int line) override;

   // CefRenderHandler overrides
  bool GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                 CefRect& rect) override;
  bool GetViewRect(CefRefPtr<CefBrowser> browser,
                           CefRect& rect) override;
  bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
                              int viewX,
                              int viewY,
                              int& screenX,
                              int& screenY) override;
  void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override {}
  void OnPopupSize(CefRefPtr<CefBrowser> browser,
                   const CefRect& rect) override {}
  void OnPaint(CefRefPtr<CefBrowser> browser,
                       PaintElementType type,
                       const RectList& dirtyRects,
                       const void* buffer,
                       int width,
                       int height) override;
  void OnCursorChange(CefRefPtr<CefBrowser> browser,
                              CefCursorHandle cursor) override;

   // CefSchemeHandlerFactory overrides
  ///
  // Return a new resource handler instance to handle the request. |browser|
  // and |frame| will be the browser window and frame respectively that
  // originated the request or NULL if the request did not originate from a
  // browser window (for example, if the request came from CefURLRequest). The
  // |request| object passed to this method will not contain cookie data.
  ///
  CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       const CefString& scheme_name,
                                       CefRefPtr<CefRequest> request) override ;

private:
   int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);
   void WindowToBrowser(int &x, int& y);

private:
   CefRefPtr<CefApp>             _app;
   CefRefPtr<CefBrowser>         _browser;

   std::mutex                    _lock;
   CursorChangeCb                _cursorChangeCb;

   int32                         _screenWidth;
   int32                         _screenHeight;
   int32                         _uiWidth;
   int32                         _uiHeight;
   uint32                        *_browserFramebuffer, *_lastBrowserFramebuffer;
   HandleRequestCb               _requestHandler;
   HWND                          _parentWindow;
   int                           _drawCount;
};

END_RADIANT_CHROMIUM_NAMESPACE

#endif // _RADIANT_CHROMIUM_BROWSER_BROWSER_H
