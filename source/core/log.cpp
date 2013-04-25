#include "radiant.h"
#include "log_internal.h"

void radiant::logger::init()
{
   FLAGS_log_dir = ".";
   google::InitGoogleLogging("tesseract");

   platform_init();
}

void radiant::log(char *str) {
   LOG(WARNING) << std::string(str ? str : "NULL");
}

void radiant::logger::exit()
{
   platform_exit();
}
