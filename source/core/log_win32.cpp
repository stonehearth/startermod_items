#include <windows.h>
#include <wincon.h>
#include <io.h>
#include <fcntl.h>

#undef ERROR
#include "radiant.h"
#include "log_internal.h"

using namespace radiant::logger;

#pragma warning(push)
#pragma warning(disable:4996)
static void redirect_io_to_console()
{
   CONSOLE_SCREEN_BUFFER_INFO coninfo;
   CONSOLE_FONT_INFOEX fontinfo;

   ::AllocConsole();

   ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
   coninfo.dwSize.X = 140;
   coninfo.dwSize.Y = 900;
   ::SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

   ::GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &fontinfo);
   wcscpy(fontinfo.FaceName, L"Lucidia Console");
   ::SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &fontinfo);

   //freopen("conin$","r", stdin);
   //freopen("conout$","w",stdout);
   //freopen("conout$","w",stderr);
}
#pragma warning(pop)


class console_logger : public google::LogSink
{
   public:
      console_logger() {
         InitializeCriticalSection(&cs);
      }

      ~console_logger() {
         DeleteCriticalSection(&cs);
      }

      // Sink's logging logic (message_len is such as to exclude '\n' at the end).
      // This method can't use LOG() or CHECK() as logging system mutex(s) are held
      // during this call.
      virtual void send(google::LogSeverity severity, const char* full_filename,
                        const char* base_filename, int line,
                        const struct ::tm* tm_time,
                        const char* message, size_t message_len) override
      {
         if (severity >= google::WARNING) {
            std::string msg = ToString(severity, base_filename, line, tm_time, message, message_len);
            // std::string msg(message, message_len);

            EnterCriticalSection(&cs);
            printf("%s\n", msg.c_str());
            LeaveCriticalSection(&cs);
         }
      }

   protected:
      CRITICAL_SECTION     cs;
};

void radiant::logger::platform_init()
{
   redirect_io_to_console();
   google::AddLogSink(new console_logger());
}

void radiant::logger::platform_exit()
{
}
