local Thread = require 'services.threads.thread'
local log = radiant.log.create_logger('threads')


local ThreadsService = class()

local Base = class()
function Base:_schedule_thread()
   self.active = true
end
function Base:get_debug_route()
   return ''
end
function Base:get_parent()
   return nil
end

function ThreadsService:__init()
   self._base = Base()
   self._main_thread = Thread(self._base):set_debug_name('main')
                               :set_thread_main(function()
                                    self:_run_main_thread()
                                 end)
   self._main_thread:start()
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._on_event_loop)
end

function ThreadsService:_run_main_thread()
   while true do
      self._main_thread:suspend()
   end
end

function ThreadsService:_on_event_loop(e)
   if self._base.active then
      self._base.active = false
      self._main_thread:_resume_thread()
   end
end

function ThreadsService:create_thread()
   return self._main_thread:create_thread()
end

function ThreadsService:get_running_thread()
   return Thread.running_thread
end

return ThreadsService()
