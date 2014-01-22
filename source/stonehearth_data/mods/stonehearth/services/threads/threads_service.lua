local Thread = require 'services.threads.thread'
local log = radiant.log.create_logger('threads')


local ThreadsService = class()

function ThreadsService:__init()
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', function()
         Thread.loop()
      end)
end

function ThreadsService:create_thread()
   return Thread.new()
end

function ThreadsService:get_current_thread()
   return Thread.current_thread
end

return ThreadsService()
