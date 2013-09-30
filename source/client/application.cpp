#include "pch.h"
#include "radiant_logger.h"
#include "radiant_json.h"
#include "client.h" 
#include "application.h"
#include "resources/res_manager.h"
#include "core/config.h"
#include <thread>

using radiant::client::Application;

void protobuf_log_handler(google::protobuf::LogLevel level, const char* filename,
                          int line, const std::string& message)
{
   LOG(INFO) << message;
}

Application::Application()
{
}
   
Application::~Application()
{
}

bool Application::LoadConfig(int argc, const char* argv[])
{
   core::Config& config = core::Config::GetInstance();

   Client::GetInstance().GetConfigOptions();
   game_engine::arbiter::GetInstance().GetConfigOptions();

   return config.Load("stonehearth", argc, argv);
}

int Application::Run(int argc, const char** argv)
{
   try {
      core::Config& config = core::Config::GetInstance();

      if (!LoadConfig(argc, argv)) {
         return 0;
      }
      radiant::logger::init(config.GetTmpDirectory() / (config.GetName() + ".log"));

      // factor this out into some protobuf helper like we did with log
      google::protobuf::SetLogHandler(protobuf_log_handler);

      // Need to load all singletons before spawning threads.
      res::ResourceManager2::GetInstance();
      client::Client::GetInstance();

      std::thread client([&]() {
         try {
            Client::GetInstance().run();
         } catch (std::exception &e) {
            LOG(WARNING) << "unhandled exception: " << e.what();
            ASSERT(false);
         }
      });

      game_engine::arbiter::GetInstance().Run();
      client.join();

      radiant::logger::exit();
   } catch (std::exception &e) {
      LOG(WARNING) << "unhandled exception: " << e.what();
      ASSERT(false);
   }
   return 0;
}
