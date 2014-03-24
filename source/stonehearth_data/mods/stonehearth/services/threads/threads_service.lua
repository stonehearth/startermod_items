local Thread = require 'services.threads.thread'
local log = radiant.log.create_logger('threads')


local ThreadsService = class()

function ThreadsService:__init()
   radiant.events.listen(radiant, 'stonehearth:gameloop', function()
         Thread.loop()
      end)
end

function ThreadsService:initialize()
end

function ThreadsService:restore(saved_variables)
end

function ThreadsService:create_thread()
   return Thread.new()
end

function ThreadsService:get_current_thread()
   return Thread.get_current_thread()
end

return ThreadsService
