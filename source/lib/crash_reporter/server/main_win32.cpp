#include "radiant.h"
#include "crash_reporter_server.h"

#include "resource.h"
#include <shellapi.h>
#include <tchar.h>

using namespace radiant::crash_reporter::server;

static std::string WstringToString(std::wstring const& input_string)
{
   return std::string(input_string.begin(), input_string.end());
}

static void GetParameters(std::string& pipe_name, std::string& dump_path, std::string& uri, std::string& userid)
{
   LPWSTR *args;
   int num_args;

   args = CommandLineToArgvW(GetCommandLineW(), &num_args);
   if (num_args >= 4) {
      pipe_name = WstringToString(args[1]);
      dump_path = WstringToString(args[2]);
      uri = WstringToString(args[3]);
      userid = WstringToString(args[4]);
   }

   if (args) {
      // Free memory allocated for CommandLineToArgvW arguments
      LocalFree(args);
   }
}

static void InitializeServer()
{
   std::string pipe_name, dump_path, uri, userid;

   GetParameters(pipe_name, dump_path, uri, userid);
   if (pipe_name.empty() || dump_path.empty() || uri.empty()) {
      throw std::invalid_argument("Invalid parameters to crash_reporter");
   }

   int const main_thread_id = GetCurrentThreadId();

   CrashReporterServer::GetInstance().Run(pipe_name, dump_path, uri, userid, [=]() {
      PostThreadMessage(main_thread_id, WM_QUIT, 0, 0);
   });
}

//--------------------------------------------------------------------------------
// Standard Windows template...

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
      // do nothing when user runs exe on their own
      return -1;
   }

   MSG msg;
   HACCEL hAccelTable;

   // Don't show a main window
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
