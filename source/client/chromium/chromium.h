#ifndef _RADIANT_CLIENT_CHROMIUM_H
#define _RADIANT_CLIENT_CHROMIUM_H

#if defined(GetNextSibling) // windows.h again... sigh.  xxx maybe WIN32_LEAN_AND_MEAN will fix this?
#  undef GetNextSibling
#  undef GetFirstChild
#endif

#include <mutex>
#include <unordered_map>
#include "include/cef_url.h"
#include "include/cef_resource_handler.h"
#include "include/cef_origin_whitelist.h"
#include "include/cef_app.h"
#include "include/cef_base.h"
#include "include/cef_client.h"
#include "include/cef_browser.h"
#include "include/cef_resource_handler.h"
#include "client/input_event.h"
#include "libjson.h"
#include "namespace.h"
#include "buffered_response.h"
#include "dm/dm.h"
#include "om/om.h"
#include "csg/region.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class ChromiumApp : public CefApp,      
                    public CefBrowserProcessHandler,
                    public CefRenderProcessHandler
{
public:
   IMPLEMENT_REFCOUNTING(ChromiumApp);

public: // CefApp overrides
};

class Chromium : public CefClient,
                 public CefLifeSpanHandler,
                 public CefSchemeHandlerFactory,
                 public CefLoadHandler,
                 public CefDisplayHandler,
                 public CefRenderHandler,
                 public CefKeyboardHandler,
                 public CefRequestHandler
{
public:
   static int Initialize();

   Chromium(HWND parentWnd);
   virtual ~Chromium();

   typedef std::function<void(const csg::Region2& rgn, const char* buffer)> PaintCb;
   typedef std::function<void(const CefCursorHandle cursor)> CursorChangeCb;

   bool HasMouseFocus();
   void UpdateDisplay(PaintCb cb);
   void Work();

   void OnRawInput(const RawInputEvent &evt, bool& handled, bool& uninstall);
   void OnMouseInput(const MouseInputEvent &evt, bool& handled, bool& uninstall);

   void SetCursorChangeCb(CursorChangeCb cb) { cursorChangeCb_ = cb; }

   typedef int CommandId;
   bool GetNextCommand(CommandId& id, JSONNode& node);
   void SetCommandResponse(CommandId& id, const JSONNode& response);

public:
   IMPLEMENT_REFCOUNTING(ChromiumApp);

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
   void OnKeyboardEvent(const RawInputEvent& kb);
   void OnMouseEvent(const MouseInputEvent& mouse);
   void OnSelectionChanged(om::EntityPtr selection);
   void ReadFile(CefRefPtr<BufferedResponse> response, std::string path);
   std::string GetPostData(CefRefPtr<CefRequest> request);
   JSONNode CommandErrorResponse(std::string reason);
   JSONNode CommandPendingReponse();
   JSONNode CommandSuccessResponse(JSONNode response);
   JSONNode CommandPendingResponse(CommandId id);
   JSONNode ExecuteAction(const JSONNode& args);
   JSONNode CreateEntity(const JSONNode& args);
   JSONNode DescribeEntities(const JSONNode& args);
   JSONNode DescribeEntity(om::EntityPtr entity, const JSONNode& types);
   void FinishCreateEntity(dm::ObjectId id, CommandId cmdId);
   int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);

private:
   HWND                          hwnd_;
   CefRefPtr<CefBrowser>         browser_;

   std::mutex                    ui_lock_;
   csg::Region2                  dirtyRegion_;
   PaintCb                       onPaint_;
   CursorChangeCb                cursorChangeCb_;

   int32                         mouseX_;
   int32                         mouseY_;
   int32                         width_;
   int32                         height_;
   int32                         renderWidth_;
   int32                         renderHeight_;
   uint32*                       framebuffer_;
   std::string                   docroot_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_CHROMIUM_H
