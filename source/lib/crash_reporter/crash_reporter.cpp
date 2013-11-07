#include "pch.h"
#include <shellapi.h>
#include <tchar.h>
#include <memory>
#include <fstream>

#include <boost/filesystem.hpp>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/StreamCopier.h"
#include "Poco/Zip/Compress.h"

#include "client/windows/crash_generation/client_info.h"

#include "crash_reporter.h"

namespace boostfs = boost::filesystem;
using namespace Poco::Net;
using namespace Poco::Zip;
using namespace google_breakpad;

static int const main_thread_id_ = GetCurrentThreadId();
static std::unique_ptr<CrashReporter> crash_reporter_;

// This is silly...
static std::wstring StringToWstring(std::string const& input_string)
{
   return std::wstring(input_string.begin(), input_string.end());
}

static std::string WstringToString(std::wstring const& input_string)
{
   return std::string(input_string.begin(), input_string.end());
}

CrashReporter::CrashReporter(std::string const& pipe_name, std::string const& dump_path, std::string const& uri) :
   pipe_name_(pipe_name),
   dump_path_(dump_path),
   uri_(uri)
{
   bool success = StartCrashGenerationServer();
   if (!success) throw new std::exception("CrashGenerationServer failed to start");
}

bool CrashReporter::StartCrashGenerationServer()
{
   std::wstring const pipe_name_w = StringToWstring(pipe_name_);
   std::wstring const dump_path_w = StringToWstring(dump_path_);

   crash_server_.reset(new CrashGenerationServer(pipe_name_w,       // name of the pipe
                                                 nullptr,           // default pipe security
                                                 OnClientConnected, // callback on client connection
                                                 this,              // context for the client connection callback
                                                 OnClientCrashed,   // callback on client crash
                                                 this,              // context for the client crashed callback
                                                 OnClientExited,    // callback on client exit
                                                 this,              // context for the client exited callback
                                                 nullptr,           // callback for upload request
                                                 nullptr,           // context for the upload request callback
                                                 true,              // generate a dump on crash
                                                 &dump_path_w));    // path to place dump files
                                                                    // fully qualified filename will be passed to the client crashed callback
                                                                    // multiple files may be generated in the case of a full memory dump request (currently off)
   return crash_server_->Start();
}

void CrashReporter::SendCrashReport(std::string const& dump_filename)
{
   std::string const zip_filename = boostfs::path(dump_filename).replace_extension(".zip").string();

   CreateZip(zip_filename, dump_filename);

   // Get the length of the zip file
   std::ifstream zip_file(zip_filename.c_str(), std::ios::in|std::ios::binary);
   zip_file.seekg(0, std::ios::end);
   long long const zip_file_length = zip_file.tellg();
   zip_file.seekg(0, std::ios::beg);

   Poco::URI uri(uri_);
   std::string path(uri.getPathAndQuery());
   HTTPClientSession session(uri.getHost(), uri.getPort());
   HTTPRequest request(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);

   request.setContentType("application/octet-stream");
   request.setContentLength(zip_file_length);

   //return; // For debugging - don't spam the test server

   // Send request, returns open stream
   std::ostream& request_stream = session.sendRequest(request);
   Poco::StreamCopier::copyStream(zip_file, request_stream);
   zip_file.close();

   // Get response
   HTTPResponse response;
   std::istream& response_stream = session.receiveResponse(response);

   // Check result
   int status = response.getStatus();
   if (status != HTTPResponse::HTTP_OK) {
      // unexpected result code
	}
   // For debugging
   std::string response_string(10000, '\0');
   response_stream.read(&response_string[0], 10000);
   MessageBox(nullptr, response_string.c_str(), "crash_reporter", MB_OK);

   // Clean up
   // Commented out during debugging
   //boostfs::remove(zip_filename); // CHECKCHECK
   //boostfs::remove(dump_filename);
}

void CrashReporter::CreateZip(std::string const& zip_filename, std::string const& dump_filename)
{
   std::ofstream zip_file(zip_filename, std::ios::binary);
   Compress encoder(zip_file, true);

   std::string const unq_dump_filename(boostfs::path(dump_filename).filename().string());
   encoder.addFile(Poco::Path(dump_filename), Poco::Path(unq_dump_filename));
   encoder.close();
}

static void RequestApplicationExit() {
   PostThreadMessage(main_thread_id_, WM_QUIT, 0, 0);
}

static void OnClientConnected(void* context, ClientInfo const* client_info)
{
}

static void OnClientCrashed(void* context, ClientInfo const* client_info, std::wstring const* dump_filename_w)
{
   CrashReporter* crash_reporter = (CrashReporter*) context;
   crash_reporter->SendCrashReport(WstringToString(*dump_filename_w));

   RequestApplicationExit();
}

static void OnClientExited(void* context, ClientInfo const* client_info)
{
   RequestApplicationExit();
}

static void GetParameters(std::string& pipe_name, std::string& dump_path, std::string& uri)
{
   LPWSTR *args;
   int num_args;

   args = CommandLineToArgvW(GetCommandLineW(), &num_args);
   if (num_args >= 4) {
      pipe_name = WstringToString(args[1]);
      dump_path = WstringToString(args[2]);
      uri = WstringToString(args[3]);
   }

   if (args) {
      // Free memory allocated for CommandLineToArgvW arguments
      LocalFree(args);
   }
}

static void InitializeServer()
{
   std::string pipe_name, dump_path, uri;

   assert(main_thread_id_ == GetCurrentThreadId());

   GetParameters(pipe_name, dump_path, uri);
   if (pipe_name.empty() || dump_path.empty() || uri.empty()) {
      throw new std::invalid_argument("Invalid parameters to CrashReporter");
   }

   crash_reporter_.reset(new CrashReporter(pipe_name, dump_path, uri));
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

   try {
      InitializeServer();
   } catch (...) {
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
