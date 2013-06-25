#include "pch.h"
#include "radiant_json.h"
#include "radiant_file.h"
#include "chromium.h"
#include "client/client.h"
#include "client/renderer/renderer.h"
#include "resources/res_manager.h"
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <fstream>

using namespace radiant;
using namespace radiant::client;

/*
class ClientOSRHandler : public CefClient,
public CefLifeSpanHandler,
public CefLoadHandler,
public CefRequestHandler,
public CefDisplayHandler,
public CefRenderHandler {
*/

static CefMainArgs main_args;
static CefRefPtr<client::ChromiumApp> chromimumApp;

///
// This function should be called from the application entry point function to
// execute a secondary process. It can be used to run secondary processes from
// the browser client executable (default behavior) or from a separate
// executable specified by the CefSettings.browser_subprocess_path value. If
// called for the browser process (identified by no "type" command-line value)
// it will return immediately with a value of -1. If called for a recognized
// secondary process it will block until the process should exit and then return
// the process exit code. The |application| parameter may be empty.
///
int Chromium::Initialize()
{
   chromimumApp = new client::ChromiumApp();
   main_args = CefMainArgs(GetModuleHandle(NULL));

   // Execute the secondary process, if any.
   // xxx - use CefSettings.browser_subprocess_path to create a ui process container with just this lib!
   return CefExecuteProcess(main_args, chromimumApp.get());
}

Chromium::Chromium(HWND parentWindow) :
   hwnd_(parentWindow),
   mouseX_(0),
   mouseY_(0)
{ 
   CefSettings settings;   
   CefString(&settings.log_file) = L"cef_debug_log.txt";
   // settings.log_severity = LOGSEVERITY_VERBOSE;
   // The multi threaded msgs loop isn't implemented on non-windows plaforms
   //settings.multi_threaded_message_loop = true; // We own the msg loop?

   settings.single_process = true; // single process mode eats nearly the entire frame time
   settings.remote_debugging_port = 1338;

   width_ = Renderer::GetInstance().GetUIWidth();
   height_ = Renderer::GetInstance().GetUIHeight();
   renderWidth_ = Renderer::GetInstance().GetWidth();
   renderHeight_ = Renderer::GetInstance().GetHeight();

   ASSERT(renderWidth_ * height_ == renderHeight_ * width_);

   CefInitialize(main_args, settings, chromimumApp.get());
   CefRegisterSchemeHandlerFactory("http", "radiant", this);

   CefWindowInfo windowInfo;
   windowInfo.SetAsOffScreen(parentWindow);
   windowInfo.SetTransparentPainting(true);

   CefBrowserSettings browserSettings;
   browserSettings.Reset();
   browserSettings.page_cache_disabled = true;
   browserSettings.application_cache_disabled = true;

   // Create the new browser window object asynchronously. This eventually results
   // in a call to CefLifeSpanHandler::OnAfterCreated().

   namespace po = boost::program_options;
   extern po::variables_map configvm;
   std::string loader = configvm["game.loader"].as<std::string>().c_str();

   json::ConstJsonObject manifest(resources::ResourceManager2::GetInstance().LookupManifest(loader));
   std::string docroot = "http://radiant/" + manifest["loader"]["ui"]["homepage"].as_string();

   CefBrowserHost::CreateBrowser(windowInfo, this, docroot, browserSettings);
   framebuffer_ = new uint32[width_ * height_];
}

void Chromium::Work()
{
   CefDoMessageLoopWork();
}

Chromium::~Chromium()
{
   CefShutdown();
   delete [] framebuffer_;
}

bool Chromium::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                             const CefPopupFeatures& popupFeatures,
                             CefWindowInfo& windowInfo,
                             const CefString& url,
                             CefRefPtr<CefClient>& client,
                             CefBrowserSettings& settings) 
{
   return false; 
}

void Chromium::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
   if (browser_ == nullptr) {
      browser_ = browser;
      //browser_->SetSize(PET_VIEW, width_, height_);

#if 0
      CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager();

      std::vector<CefString> schemes;
      schemes.push_back("file");
      schemes.push_back("radiant");

      manager->SetSupportedSchemes(schemes);
#endif
   }

}

bool Chromium::RunModal(CefRefPtr<CefBrowser> browser) 
{ 
   return false; 
}

bool Chromium::DoClose(CefRefPtr<CefBrowser> browser) 
{ 
   return false;
}

void Chromium::OnBeforeClose(CefRefPtr<CefBrowser> browser) 
{
}

void Chromium::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)  
{
   if (!browser->IsPopup() && frame->IsMain()) {
      // We've just started loading a page
   }
}

void Chromium::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) 
{
   if (!browser->IsPopup() && frame->IsMain()) {
   }
}

// CefDisplayHandler methods
void Chromium::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) 
{
}

void Chromium::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
}  

bool Chromium::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString& message, const CefString& source, int line) 
{
   if (message.c_str()) {
      char msg[128];
      std::wcstombs(msg, message.c_str(), sizeof msg);

      LOG(WARNING) << "chromium: " << msg;
   }
   return false;
}

// CefRenderHandler overrides
bool Chromium::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
                                 CefRect& rect)
{
   rect.Set(0, 0, width_, height_);
   return true;
}

bool Chromium::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) 
{
   rect.Set(0, 0, width_, height_);
   return true;
}

bool Chromium::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) 
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

void Chromium::OnPaint(CefRefPtr<CefBrowser> browser,
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

void Chromium::UpdateDisplay(PaintCb cb)
{
   std::lock_guard<std::mutex> guard(ui_lock_);

   cb(dirtyRegion_, (const char*)framebuffer_);
   dirtyRegion_.Clear();
}

int Chromium::GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam)
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

void Chromium::OnRawInput(const RawInputEvent &msg, bool& handled, bool& uninstall)
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

void Chromium::OnMouseInput(const MouseInputEvent& mouse, bool& handled, bool& uninstall)
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
   evt.x = mouseX_ = mouse.x * width_ / renderWidth_ ;
   evt.y = mouseY_ = mouse.y * height_ / renderHeight_;
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

void Chromium::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor)
{
   if (cursorChangeCb_) {
      cursorChangeCb_(cursor);
   }
}

// xxx - nuke this...
bool Chromium::HasMouseFocus()
{
   if (mouseX_ < 0 || mouseY_ < 0 || mouseX_ >= width_ || mouseY_ >= height_) {
      return false;
   }
   uint32 pixel = framebuffer_[mouseX_ + (mouseY_ * width_)];
   return ((pixel >> 24) && 0xff) != 0;
}

CefRefPtr<CefResourceHandler> Chromium::GetResourceHandler(CefRefPtr<CefBrowser> browser,
                                                           CefRefPtr<CefFrame> frame,
                                                           CefRefPtr<CefRequest> request)
{
   std::string url = request->GetURL().ToString();
   
   CefURLParts url_parts;
   CefParseURL(url, url_parts);

   CefRefPtr<BufferedResponse> response(new BufferedResponse(request));
   auto cb = [response](std::string const& node) {
      // is it ok to do this on the client thread???
      response->SetResponse(node, "application/json");
   };

   auto &api = Client::GetInstance().GetAPI();


   std::string path = CefString(&url_parts.path);
   std::string query = CefString(&url_parts.query);
   std::string postdata = GetPostData(request);

   if (!api.OnNewRequest(path, query, postdata, cb)) {
      ReadFile(response, CefString(&url_parts.path));
   }
   return CefRefPtr<CefResourceHandler>(response);
}


CefRefPtr<CefResourceHandler> Chromium::Create(CefRefPtr<CefBrowser> browser,
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

void Chromium::OnProtocolExecution(CefRefPtr<CefBrowser> browser,
                                   const CefString& url,
                                   bool& allow_os_execution)
{
   std::string uri = url.ToString();
   allow_os_execution = boost::starts_with(uri, "http://radiant/");
}
//
//void Chromium::OnResourceResponse(CefRefPtr<CefBrowser> browser,
//                                  const CefString& url,
//                                  CefRefPtr<CefResponse> response,
//                                  CefRefPtr<CefContentFilter>& filter)
//{
//   CefResponse::HeaderMap hm;
//   std::string uri = url.ToString();
//   std::string text = response->GetStatusText();
//   std::string mime = response->GetMimeType();
//   int code = response->GetStatus();
//   response->GetHeaderMap(hm);
//   for (auto& entry: hm) {
//      //LOG(WARNING) << entry.first.ToString() << ": " << entry.second.ToString();
//   }
//}
//

void Chromium::ReadFile(CefRefPtr<BufferedResponse> response, std::string path)
{
   static const struct {
      char *extension;
      char *mimeType;
   } mimeTypes_[] = {
     { "htm",  "text/html" },
     { "html", "text/html" },
     { "css",  "text/css" },
     { "js",   "application/x-javascript" },
     { "json", "application/json" },
     { "txt",  "text/plain" },
     { "jpg",  "image/jpeg" },
     { "png",  "image/png" },
     { "gif",  "image/gif" },
     { "woff", "application/font-woff" },
     { "cur",  "image/vnd.microsoft.icon" },
   };
   std::ifstream infile;
   auto const& rm = resources::ResourceManager2::GetInstance();

   std::string data;
   try {
      if (boost::ends_with(path, ".json")) {
         JSONNode const& node = rm.LookupJson(path);
         data = node.write();
      } else {
         rm.OpenResource(path, infile);
         data = io::read_contents(infile);
      }
   } catch (resources::Exception const& e) {
      LOG(WARNING) << "error code 404: " << e.what();
      response->SetStatusCode(404);
      return;
   }


   // Determine the file extension.
   std::string mimeType;
   std::size_t last_dot_pos = path.find_last_of(".");
   if (last_dot_pos != std::string::npos) {
      std::string extension = path.substr(last_dot_pos + 1);
      for (auto &entry : mimeTypes_) {
         if (extension == entry.extension) {
            mimeType = entry.mimeType;
            break;
         }
      }
   }
   ASSERT(!mimeType.empty());
   response->SetResponse(data, mimeType);
}

std::string Chromium::GetPostData(CefRefPtr<CefRequest> request)
{
   CefRequest::HeaderMap headerMap;
   request->GetHeaderMap(headerMap);

   CefRequest::HeaderMap::iterator it = headerMap.find("Content-Type");

   JSONNode result;
   auto postData = request->GetPostData();
   if (postData) {
      int c = postData->GetElementCount();
      if (c) {
         CefPostData::ElementVector elements;
         postData->GetElements(elements);
         for (auto &e : elements) {
            int count = e->GetBytesCount();
            char* bytes = new char[count];           
            e->GetBytes(count, bytes);
            
            return std::string(bytes, count);
         }
      }
   }
   return std::string();
}
