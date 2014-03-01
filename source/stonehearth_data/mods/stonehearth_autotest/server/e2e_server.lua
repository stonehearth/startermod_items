{
   "info" : {
      "name" : "Autotest: End to End Automated Testing"
   },

   "server_init_script" : "file(autotest_end_to_end_server)",

   "functions" : {
      "get_world_generation_progress": {
         "controller" : "file(call_handlers/world_generation_call_handler.lua)",
         "endpoint" : "server"
      }
   },

  "ui" :  {
      "homepage" : "http://radiant/stonehearth/ui/root/index.html?skip_title=true)",
      "js" : [
         "file(server/ui_automation_hook.js)"
         "file(ui/test_progress/test_progress.js)"
      ],
   }
}
