local WorldGenerationCallHandler = class()
local log = radiant.log.create_logger('world_generation')

local progress = nil

function WorldGenerationCallHandler:get_world_generation_progress(session, request) 
   if not progress then
      progress = radiant.create_datastore()
      radiant.events.listen(radiant.events, 'stonehearth:generate_world_progress', function(e)
            progress:update({
               progress = e.progress
            })
            if e.progress == 100 then
               return radiant.events.UNLISTEN
            end
         end)
      progress:update({});
   end
   return { tracker = progress }
end

return WorldGenerationCallHandler
