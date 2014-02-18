#include "radiant.h"
#include "radiant_file.h"
#include "core/config.h"
#include "core/system.h"
#include "lib/rpc/http_deferred.h"
#include "chromium/cef_headers.h"
#include "chromium/app/app.h"
#include "browser.h"
#include "response.h"
#include "lib/perfmon/perfmon.h"
#include <fstream>
#include <boost/algorithm/string.hpp>

#define BROWSER_LOG(level)    LOG(browser, level)

using namespace radiant;
using namespace radiant::chromium;

static std::string GetPostData(CefRefPtr<CefRequest> request);
static JSONNode GetQuery(std::string query);

IBrowser* ::radiant::chromium::CreateBrowser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port)
{
   return new Browser(parentWindow, docroot, width, height, debug_port);
}

Browser::Browser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port) :
   _screenWidth(width),
   _screenHeight(height)
{ 
   _uiWidth = 1920;
   _uiHeight = 1080;
   if (_screenWidth >= 1920 && _screenHeight >= 1080) {
      _uiWidth = _screenWidth;
      _uiHeight = _screenHeight;
   }
   _browserFramebuffer = 0x0;
   _lastBrowserFramebuffer = 0x0;
   _drawCount = 0;

   CefMainArgs main_args(GetModuleHandle(NULL));
   if (!CefExecuteProcess(main_args, _app)) {
      ASSERT(false);
   }

   CefSettings settings;   

   CefString(&settings.log_file) = (core::System::GetInstance().GetTempDirectory() / "chromium.log").wstring().c_str();   
   settings.log_severity = LOGSEVERITY_DISABLE;
   settings.single_process = false; // single process mode eats nearly the entire frame time
   settings.remote_debugging_port = debug_port;

   CefInitialize(main_args, settings, _app.get());
   CefRegisterSchemeHandlerFactory("http", "radiant", this);
   CefWindowInfo windowInfo;
   windowInfo.SetAsOffScreen(parentWindow);
   windowInfo.SetTransparentPainting(true);

   CefBrowserSettings browserSettings;
   browserSettings.Reset();
   // browserSettings.developer_tools = STATE_ENABLED;
   browserSettings.java = STATE_DISABLED;
   browserSettings.plugins = STATE_DISABLED;

   CefBrowserHost::CreateBrowser(windowInfo, this, docroot, browserSettings);
}

void Browser::Work()
{
   CefDoMessageLoopWork();
}

Browser::~Browser()
{
   CefShutdown();
}

bool Browser::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                             const CefPopupFeatures& popupFeatures,
                             CefWindowInfo& windowInfo,
                             const CefString& url,
                             CefRefPtr<CefClient>& client,
                             CefBrowserSettings& settings) 
{
   return false; 
}

void Browser::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
   if (_browser == nullptr) {
      _browser = browser;
      _browser->GetHost()->NotifyScreenInfoChanged();
      //browser_->SetSize(PET_VIEW, uiWidth_, uiHeight_);
   }
}

bool Browser::RunModal(CefRefPtr<CefBrowser> browser) 
{ 
   return false; 
}

bool Browser::DoClose(CefRefPtr<CefBrowser> browser) 
{ 
   return false;
}

void Browser::OnBeforeClose(CefRefPtr<CefBrowser> browser) 
{
}

void Browser::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)  
{
   if (!browser->IsPopup() && frame->IsMain()) {
      // We've just started loading a page
   }
}

void Browser::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) 
{
   if (!browser->IsPopup() && frame->IsMain()) {
   }
}

// CefDisplayHandler methods
void Browser::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) 
{
}

void Browser::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
}  

bool Browser::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString& message, const CefString& source, int line) 
{
   if (message.c_str()) {
      char msg[128];
      std::wcstombs(msg, message.c_str(), sizeof msg);

      BROWSER_LOG(4) << "console log: " << msg;
   }
   return false;
}

// CefRenderHandler overrides
bool Browser::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                 CefRect& rect)
{
   rect.Set(0, 0, _uiWidth, _uiHeight);
   return true;
}

bool Browser::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) 
{
   rect.Set(0, 0, _uiWidth, _uiHeight);
   return true;
}

bool Browser::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) 
{
   screenX = viewX;
   screenY = viewY;
   return true;
}

// Called when an element should be painted. |type| indicates whether the
// element is the view or the popup widget. |buffer| contains the pixel data
// for the whole image. |dirtyRects| contains the set of rectangles that need
// to be repainted. On Windows |buffer| will be |width|*|height|*4 bytes
// in size and represents a BGRA image with an upper-left origin.

void Browser::OnPaint(CefRefPtr<CefBrowser> browser,
                       PaintElementType type,
                       const RectList& dirtyRects,
                       const void* buffer,
                       int width,
                       int height)
{
   perfmon::SwitchToCounter("copy chromium fb to system buffer") ;
   std::lock_guard<std::mutex> guard(_lock);

   // xxx: we can optimize this by accumulating a dirty region and sending a message to
   // the ui thread to do the copy once every frame -- unknown
   //
   // we actually can't, as the backing store in 'buffer' may get freed after this call
   // for all we know! -- tony

   if (width != _uiWidth || height != _uiHeight) {
      // for some reason, the dimensions of the buffer passed in do not match the
      // size of our backing store.  just ignore this paint.  this can sometimes happen
      // when the window is resized when we get paints from the old (and out of date)
      // size.
      BROWSER_LOG(3) << "ignoring paint request from browser (" 
                     << csg::Point2(width, height) << " != "
                     << csg::Point2(_uiWidth, _uiHeight) << ")";
      return;
   }

   _drawCount++;
   if (_browserFramebuffer == nullptr) {
      return;
   }

   memcpy(_browserFramebuffer, buffer, width * height * 4);
}

void Browser::UpdateBrowserFrambufferPtrs(unsigned int* last_written, unsigned int* next_to_write)
{
   _lastBrowserFramebuffer = last_written;
   _browserFramebuffer = next_to_write;
}

void Browser::UpdateDisplay(PaintCb cb)
{
   std::lock_guard<std::mutex> guard(_lock);
 
   if (_drawCount > 0) {
      // Always inform clients that our entire view has changed.  This is because it's possible
      // that our OnPaint method can be called more than once before UpdateDisplay is polled, leading
      // to outdated bounding rect information being sent to any clients.
      csg::Region2 r(csg::Rect2(csg::Point2(0, 0), csg::Point2(_uiWidth, _uiHeight)));

      cb(r);
      _drawCount = 0;
   }
}

int Browser::GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam)
{
   auto isKeyDown = [](WPARAM arg) {
      return (GetKeyState(arg) & 0x8000) != 0;
   };

   int modifiers = 0;
   if (isKeyDown(VK_SHIFT))
      modifiers |= EVENTFLAG_SHIFT_DOWN;
   if (isKeyDown(VK_CONTROL))
      modifiers |= EVENTFLAG_CONTROL_DOWN;
   if (isKeyDown(VK_MENU))
      modifiers |= EVENTFLAG_ALT_DOWN;

   // Low bit set from GetKeyState indicates "toggled".
   if (::GetKeyState(VK_NUMLOCK) & 1)
      modifiers |= EVENTFLAG_NUM_LOCK_ON;
   if (::GetKeyState(VK_CAPITAL) & 1)
      modifiers |= EVENTFLAG_CAPS_LOCK_ON;

   switch (wparam) {
   case VK_RETURN:
      if ((lparam >> 16) & KF_EXTENDED)
         modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
   case VK_INSERT:
   case VK_DELETE:
   case VK_HOME:
   case VK_END:
   case VK_PRIOR:
   case VK_NEXT:
   case VK_UP:
   case VK_DOWN:
   case VK_LEFT:
   case VK_RIGHT:
      if (!((lparam >> 16) & KF_EXTENDED))
         modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
   case VK_NUMLOCK:
   case VK_NUMPAD0:
   case VK_NUMPAD1:
   case VK_NUMPAD2:
   case VK_NUMPAD3:
   case VK_NUMPAD4:
   case VK_NUMPAD5:
   case VK_NUMPAD6:
   case VK_NUMPAD7:
   case VK_NUMPAD8:
   case VK_NUMPAD9:
   case VK_DIVIDE:
   case VK_MULTIPLY:
   case VK_SUBTRACT:
   case VK_ADD:
   case VK_DECIMAL:
   case VK_CLEAR:
      modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
   case VK_SHIFT:
      if (isKeyDown(VK_LSHIFT))
         modifiers |= EVENTFLAG_IS_LEFT;
      else if (isKeyDown(VK_RSHIFT))
         modifiers |= EVENTFLAG_IS_RIGHT;
      break;
   case VK_CONTROL:
      if (isKeyDown(VK_LCONTROL))
         modifiers |= EVENTFLAG_IS_LEFT;
      else if (isKeyDown(VK_RCONTROL))
         modifiers |= EVENTFLAG_IS_RIGHT;
      break;
   case VK_MENU:
      if (isKeyDown(VK_LMENU))
         modifiers |= EVENTFLAG_IS_LEFT;
      else if (isKeyDown(VK_RMENU))
         modifiers |= EVENTFLAG_IS_RIGHT;
      break;
   case VK_LWIN:
      modifiers |= EVENTFLAG_IS_LEFT;
      break;
   case VK_RWIN:
      modifiers |= EVENTFLAG_IS_RIGHT;
      break;
   }
   return modifiers;
}

bool Browser::OnInput(Input const& evt)
{
   if (evt.type == evt.MOUSE) {
      OnMouseInput(evt.mouse);
   } else if (evt.type == evt.RAW_INPUT) {
      OnRawInput(evt.raw_input);
   }
   return true;
}

void Browser::OnRawInput(const RawInput &msg)
{
   if (!_browser) {
      return;
   }

   CefKeyEvent evt;

   switch (msg.msg) {
   case WM_KEYDOWN:
   case WM_SYSKEYDOWN:
   case WM_KEYUP:
   case WM_SYSKEYUP:
   case WM_SYSCHAR:
   case WM_CHAR:
   case WM_IME_CHAR: {
      evt.windows_key_code = msg.wp;
      evt.native_key_code = msg.lp;
      evt.is_system_key = msg.msg == WM_SYSCHAR || msg.msg == WM_SYSKEYDOWN || msg.msg == WM_SYSKEYUP;
      if (msg.msg == WM_KEYDOWN || msg.msg == WM_SYSKEYDOWN) {
         evt.type = KEYEVENT_RAWKEYDOWN;
      } else if (msg.msg == WM_KEYUP || msg.msg == WM_SYSKEYUP) {
         evt.type = KEYEVENT_KEYUP;
      } else {
         evt.type = KEYEVENT_CHAR;
      }
      evt.modifiers = GetCefKeyboardModifiers(msg.wp, msg.lp);
      BROWSER_LOG(7) << "sending key " << ((char)evt.windows_key_code) << " " << evt.type;

      _browser->GetHost()->SendKeyEvent(evt);
      break;
      }
   }
}

void Browser::SetCursorChangeCb(CursorChangeCb cb) {
   _cursorChangeCb = cb;
}

void Browser::OnMouseInput(const MouseInput& mouse)
{
   static const CefBrowserHost::MouseButtonType types[] = {
      MBT_LEFT,
      MBT_RIGHT,
      MBT_MIDDLE,
   };

   if (!_browser) {
      return;
   }
   int x = mouse.x;
   int y = mouse.y;
   WindowToBrowser(x, y);

   CefMouseEvent evt;
   evt.x = x;
   evt.y = y;
   evt.modifiers = 0;

   auto host = _browser->GetHost();
   host->SendMouseMoveEvent(evt, false);
   BROWSER_LOG(7) << "sending mouse move " << x << ", " << y << ".";
   
   for (auto type : types) {
      // xxx: until i have time to fix the "right click interpreted as middle click",
      // just send left clicks.
      if (type == MBT_LEFT) {
         if (mouse.down[type]) {
            host->SendMouseClickEvent(evt, type, false, 1);
         }
         if (mouse.up[type]) {
            host->SendMouseClickEvent(evt, type, true, 1);
         }
      }
   }
}

void Browser::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor)
{
   if (_cursorChangeCb) {
      _cursorChangeCb(cursor);
   }
}

// xxx - nuke this...
bool Browser::HasMouseFocus(int x, int y)
{
   if (_lastBrowserFramebuffer == nullptr) {
      return false;
   }
   WindowToBrowser(x, y);

   if (x < 0 || y < 0 || x >= _uiWidth || y >= _uiHeight) {
      return false;
   }
   uint32 pixel = _lastBrowserFramebuffer[x + (y * _uiWidth)];
   return ((pixel >> 24) && 0xff) != 0;
}

CefRefPtr<CefResourceHandler> Browser::GetResourceHandler(CefRefPtr<CefBrowser> browser,
                                                          CefRefPtr<CefFrame> frame,
                                                          CefRefPtr<CefRequest> request)
{
   if (!_requestHandler) {
      return nullptr;
   }

   const CefString url = request->GetURL();
   
   CefURLParts url_parts;
   CefParseURL(url, url_parts);
   std::string path = CefString(&url_parts.path);
   JSONNode query = GetQuery(CefString(&url_parts.query));
   std::string postdata = GetPostData(request);

   // Create a new Response object.  Response implements CefResourceHandler and will
   // be used to communicate the response to chrome.
   Response *response = new Response(request);

   // Hold a reference to the response in an HttpDeferred deferred object.  This
   // will be used to interface with the rest of the system.  Chrome will be handed
   // the data whenever someone calls Complete.
   response->AddRef();
   std::shared_ptr<rpc::HttpDeferred> deferred(new rpc::HttpDeferred(CefString(url)), [=](rpc::HttpDeferred* p) {
      response->Release();
      delete p;
   });
   deferred->Done([=](rpc::HttpResponse const& r) {
      response->SetResponse(r.code, r.content, r.mime_type);
   });
   deferred->Fail([=](rpc::HttpError const& r) {
      response->SetResponse(r.code, r.response, "text/plain");
   });

   _requestHandler(path, query, postdata, deferred);

   return CefRefPtr<CefResourceHandler>(response);
}


CefRefPtr<CefResourceHandler> Browser::Create(CefRefPtr<CefBrowser> browser,
                                               CefRefPtr<CefFrame> frame,
                                               const CefString& scheme_name,
                                               CefRefPtr<CefRequest> request)
{
   return GetResourceHandler(browser, frame, request);
}


// Called on the IO thread to handle requests for URLs with an unknown
// protocol component. Return true to indicate that the request should
// succeed because it was handled externally. Set |allowOSExecution| to true
// and return false to attempt execution via the registered OS protocol
// handler, if any. If false is returned and either |allow_os_execution|
// is false or OS protocol handler execution fails then the request will fail
// with an error condition.
// SECURITY WARNING: YOU SHOULD USE THIS METHOD TO ENFORCE RESTRICTIONS BASED
// ON SCHEME, HOST OR OTHER URL ANALYSIS BEFORE ALLOWING OS EXECUTION.

void Browser::OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                   const CefString& url,
                                   bool& allow_os_execution)
{
   std::string uri = url.ToString();
   allow_os_execution = boost::starts_with(uri, "http://radiant/");
}


static std::string UrlDecode(std::string const& in)
{
   std::string out;
   out.reserve(in.size());
   for (std::size_t i = 0; i < in.size(); ++i) {
      if (in[i] == '%') {
         if (i + 3 > in.size()) {
            // xxx: error!
            break;
         }
         int value = 0;
         std::istringstream is(in.substr(i + 1, 2));
         is >> std::hex >> value;
         out += static_cast<char>(value);
         i += 2;
      } else if (in[i] == '+') {
         out += ' ';
      } else {
         out += in[i];
      }
   }
   return out;
}

static JSONNode GetQuery(std::string query)
{
   JSONNode result;
   size_t start = 0, end;
   do {
      end = query.find('&', start);
      if (end == std::string::npos) {
         end = query.size();
      }
      if (end > start) {
         size_t mid = query.find_first_of('=', start);
         if (mid != std::string::npos) {
            std::string name = query.substr(start, mid - start);
            std::string value = query.substr(mid + 1, end - mid - 1);

            name = UrlDecode(name);
            value = UrlDecode(value);
            int ivalue = atoi(value.c_str());
            if (ivalue != 0 || value == "0") {
               result.push_back(JSONNode(name, ivalue));
            } else {
               result.push_back(JSONNode(name, value));
            }
         }
         start = end + 1;
      }
   } while (end < query.size());
   return result;
}

static std::string GetPostData(CefRefPtr<CefRequest> request)
{
   std::string result;

   auto postData = request->GetPostData();
   if (postData) {
      int c = postData->GetElementCount();
      if (c == 1) {
         CefPostData::ElementVector elements;
         postData->GetElements(elements);
         auto const e = elements[0];
         int count = e->GetBytesCount();
         char* bytes = new char[count];           
         e->GetBytes(count, bytes);
         std::string result(bytes, count);
         delete [] bytes;

         return result;
      }
   }
   return std::string();
}

void Browser::SetRequestHandler(HandleRequestCb cb)
{
   _requestHandler = cb;
}

void Browser::WindowToBrowser(int& x, int& y) 
{
   x = (int)((x / (float)_screenWidth) * _uiWidth);
   y = (int)((y / (float)_screenHeight) * _uiHeight);
}

void Browser::OnScreenResize(int w, int h)
{
   if (_screenWidth == w && _screenHeight == h)
   {
      return;
   }
   _screenWidth = w;
   _screenHeight = h;

   if (_screenWidth >= 1920 && _screenHeight >= 1080) {
      _uiWidth = _screenWidth;
      _uiHeight = _screenHeight;
   } else {
      _uiWidth = 1920;
      _uiHeight = 1080;
   }

   BROWSER_LOG(3) << "Browser (UI) resized to " << _uiWidth << "x" << _uiHeight;

   if (_browser) {
      _browser->GetHost()->NotifyScreenInfoChanged();
   }
}

void Browser::GetBrowserSize(int& w, int& h)
{
   w = _uiWidth;
   h = _uiHeight;
}

