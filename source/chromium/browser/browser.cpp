#include "pch.h"

#if 0
#include "radiant_json.h"
#endif

#include "radiant_file.h"
#include "chromium/app/app.h"
#include "browser.h"
#include "response.h"
#include <fstream>

using namespace radiant;
using namespace radiant::chromium;

static std::string GetPostData(CefRefPtr<CefRequest> request);
static JSONNode GetQuery(std::string query);
/*
class ClientOSRHandler : public CefClient,
public CefLifeSpanHandler,
public CefLoadHandler,
public CefRequestHandler,
public CefDisplayHandler,
public CefRenderHandler {
*/

IBrowser* ::radiant::chromium::CreateBrowser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port)
{
   return new Browser(parentWindow, docroot, width, height, debug_port);
}

Browser::Browser(HWND parentWindow, std::string const& docroot, int width, int height, int debug_port) :
   hwnd_(parentWindow),
   width_(width),
   height_(height),
   mouseX_(0),
   mouseY_(0)
{ 
   framebuffer_ = new uint32[width_ * height_];

   CefMainArgs main_args(GetModuleHandle(NULL));
   if (!CefExecuteProcess(main_args, app_)) {
      ASSERT(false);
   }

   CefSettings settings;   

   CefString(&settings.log_file) = L"cef_debug_log.txt";
   settings.log_severity = LOGSEVERITY_VERBOSE;
   settings.single_process = false; // single process mode eats nearly the entire frame time
   settings.remote_debugging_port = debug_port;


   CefInitialize(main_args, settings, app_.get());
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
   delete [] framebuffer_;
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
   if (browser_ == nullptr) {
      browser_ = browser;
      //browser_->SetSize(PET_VIEW, width_, height_);
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

      LOG(WARNING) << "chromium: " << msg;
   }
   return false;
}

// CefRenderHandler overrides
bool Browser::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                 CefRect& rect)
{
   rect.Set(0, 0, width_, height_);
   return true;
}

bool Browser::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) 
{
   rect.Set(0, 0, width_, height_);
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
   std::lock_guard<std::mutex> guard(ui_lock_);

   // xxx: we can optimze this by accuulating a dirty region and sending a message to
   // the ui thread to do the copy once every frame

   const uint32* src = (const uint32*)buffer;
   for (auto& r : dirtyRects) {
      int offset = r.x + (r.y * width_);
      for (int dy = 0; dy < r.height; dy++) {
         memcpy(framebuffer_ + offset, src + offset, r.width * 4);
         offset += width_;
      }
      dirtyRegion_ += csg::Rect2(csg::Point2(r.x, r.y), csg::Point2(r.x + r.width, r.y + r.height));
   }
}

void Browser::UpdateDisplay(PaintCb cb)
{
   std::lock_guard<std::mutex> guard(ui_lock_);

   cb(dirtyRegion_, (const char*)framebuffer_);
   dirtyRegion_.Clear();
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

void Browser::OnRawInput(const RawInputEvent &msg, bool& handled, bool& uninstall)
{
   if (!browser_) {
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
      LOG(WARNING) << " -------- SENDING KEY " << ((char)evt.windows_key_code) << " " << evt.type << " --------";

      browser_->GetHost()->SendKeyEvent(evt);
      break;
      }
   }
}

void Browser::SetCursorChangeCb(CursorChangeCb cb) {
   cursorChangeCb_ = cb;
}

void Browser::OnMouseInput(const MouseEvent& mouse, bool& handled, bool& uninstall)
{
   static const CefBrowserHost::MouseButtonType types[] = {
      MBT_LEFT,
      MBT_RIGHT,
      MBT_MIDDLE,
   };

   if (!browser_) {
      return;
   }

   CefMouseEvent evt;
   evt.x = mouseX_ = mouse.x;
   evt.y = mouseY_ = mouse.y;
   evt.modifiers = 0;

   auto host = browser_->GetHost();
   host->SendMouseMoveEvent(evt, false);
   //LOG(WARNING) << "sending mouse move " << mouseX_ << ", " << mouseY_ << ".";
   
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
   if (cursorChangeCb_) {
      cursorChangeCb_(cursor);
   }
}

// xxx - nuke this...
bool Browser::HasMouseFocus()
{
   if (mouseX_ < 0 || mouseY_ < 0 || mouseX_ >= width_ || mouseY_ >= height_) {
      return false;
   }
   uint32 pixel = framebuffer_[mouseX_ + (mouseY_ * width_)];
   return ((pixel >> 24) && 0xff) != 0;
}

CefRefPtr<CefResourceHandler> Browser::GetResourceHandler(CefRefPtr<CefBrowser> browser,
                                                          CefRefPtr<CefFrame> frame,
                                                          CefRefPtr<CefRequest> request)
{
   if (!requestHandler_) {
      return nullptr;
   }

   std::string url = request->GetURL().ToString();
   
   CefURLParts url_parts;
   CefParseURL(url, url_parts);
   std::string path = CefString(&url_parts.path);
   JSONNode query = GetQuery(CefString(&url_parts.query));
   std::string postdata = GetPostData(request);

   Response *r = new Response(request);
   r->AddRef();
   std::shared_ptr<net::IResponse> response(r, [](Response* p) {
      p->Release();
   });

   requestHandler_(path, query, postdata, response);

   return CefRefPtr<CefResourceHandler>(r);
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
   requestHandler_ = cb;
}
