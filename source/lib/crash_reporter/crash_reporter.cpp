#include "pch.h"
#include <shellapi.h>
#include <tchar.h>
#include <memory>
#include <fstream>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;

#include "client/windows/crash_generation/client_info.h"
#include "client/windows/crash_generation/crash_generation_server.h"

#include "crash_reporter.h"

using namespace google_breakpad;

static int const main_thread_id = GetCurrentThreadId();
static std::unique_ptr<CrashGenerationServer> crash_server;

static void RequestApplicationExit() {
   PostThreadMessage(main_thread_id, WM_QUIT, 0, 0);
}

static void OnClientConnected(void* context, const ClientInfo* client_info)
{
}

static void OnClientCrashed(void* context, const ClientInfo* client_info, const wstring* dumpfile_name)
{
   // Get the length of the dumpfile
   std::ifstream dump_file(dumpfile_name->c_str(), std::ifstream::in|std::ifstream::binary);
   dump_file.seekg(0, std::ifstream::end);
   long long const file_length = dump_file.tellg();
   dump_file.seekg(0, std::ifstream::beg);

   std::string const domain = "posttestserver.com";
   std::string const path = "/post.php";

   HTTPClientSession session(domain);
   HTTPRequest request(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);

   request.setContentType("application/octet-stream");
   request.setContentLength(file_length);

   // Send request, returns open stream
   std::ostream& request_stream = session.sendRequest(request);
   Poco::StreamCopier::copyStream(dump_file, request_stream);
   dump_file.close();

   // Get response
   HTTPResponse response;
   std::istream& response_stream = session.receiveResponse(response);

   // Check result
   int status = response.getStatus();
   if (status != HTTPResponse::HTTP_OK) {
      // unexpected result code
      //std::string response_string(100000, '\0');
      //response_stream.read(&response_string[0], 100000);
      //MessageBox(NULL, response_string.data(), "crash_reporter", MB_OK); // CHECKCHECK
	}

   RequestApplicationExit();
}

static void OnClientExited(void* context, const ClientInfo* client_info)
{
   RequestApplicationExit();
}

static bool CrashServerStart(std::wstring const& pipe_name, std::wstring const& dump_path)
{
   crash_server.reset(new CrashGenerationServer(pipe_name,         // name of the pipe
                                                nullptr,           // default pipe security
                                                OnClientConnected, // callback on client connection
                                                nullptr,           // context for the client connection callback
                                                OnClientCrashed,   // callback on client crash
                                                nullptr,           // contect for the client crashed callback
                                                OnClientExited,    // callback on client exit
                                                nullptr,           // context for the client exited callback
                                                nullptr,           // callback for upload request
                                                nullptr,           // context for the upload request callback
                                                true,              // generate a dump on crash
                                                &dump_path));      // path to place dump files
                                                                   // fully qualified filename will be passed to the client crashed callback
                                                                   // multiple files may be generated in the case of a full memory dump request (currently off)

   return crash_server->Start();
}

static void GetParameters(std::wstring& pipe_name, std::wstring& dump_path)
{
   LPWSTR *args;
   int num_args;

   args = CommandLineToArgvW(GetCommandLineW(), &num_args);
   if (num_args >= 3) {
      pipe_name = args[1];
      dump_path = args[2];
   }

   if (args) {
      // Free memory allocated for CommandLineToArgvW arguments
      LocalFree(args);
   }
}

static bool InitializeServer()
{
   std::wstring pipe_name, dump_path;

   assert(main_thread_id == GetCurrentThreadId());

   GetParameters(pipe_name, dump_path);
   if (pipe_name.empty() || dump_path.empty()) return false;

   if (!CrashServerStart(pipe_name, dump_path)) return false;
   return true;
}

//--------------------------------------------------------------------------------

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];               // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];         // the main window class name

// Forward declarations of functions included in this code module:
ATOM            MyRegisterClass(HINSTANCE hInstance);
BOOL            InitInstance(HINSTANCE, int);
LRESULT CALLBACK   WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK   About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   // TODO: Place code here.
   if (!InitializeServer()) {
      return -1;
   }

   MSG msg;
   HACCEL hAccelTable;

   /*
   // Initialize global strings
   LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
   LoadString(hInstance, IDC_CRASH_REPORTER, szWindowClass, MAX_LOADSTRING);
   MyRegisterClass(hInstance);

   // Perform application initialization:
   if (!InitInstance (hInstance, nCmdShow)) {
      return FALSE;
   }
   */
   hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CRASH_REPORTER));

   // Main message loop:
   while (GetMessage(&msg, nullptr, 0, 0)) {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
   WNDCLASSEX wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style         = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc   = WndProc;
   wcex.cbClsExtra      = 0;
   wcex.cbWndExtra      = 0;
   wcex.hInstance      = hInstance;
   wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CRASH_REPORTER));
   wcex.hCursor      = LoadCursor(nullptr, IDC_ARROW);
   wcex.hbrBackground   = (HBRUSH)(COLOR_WINDOW+1);
//   wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_CRASH_REPORTER);
   wcex.lpszMenuName   = nullptr;
   wcex.lpszClassName   = szWindowClass;
   wcex.hIconSm      = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

   return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 320, 240, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND   - process the application menu
//  WM_PAINT   - Paint the main window
//  WM_DESTROY   - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;
   PAINTSTRUCT ps;
   HDC hdc;

   switch (message) {
   case WM_COMMAND:
      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      // Parse the menu selections:
      switch (wmId) {
      case IDM_EXIT:
         DestroyWindow(hWnd);
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
   case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      // TODO: Add any drawing code here...
      EndPaint(hWnd, &ps);
      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
