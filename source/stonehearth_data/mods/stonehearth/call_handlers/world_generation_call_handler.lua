local WorldGenerationCallHandler = class()

local progress = nil

function WorldGenerationCallHandler:get_world_generation_progress(session, request)
   
   if not progress then
      progress = _radiant.sim.create_data_store()
      radiant.events.listen(radiant.events, 'stonehearth:generate_world_progress', self, self.update_progress)
      progress:update({});
   end

   return { tracker = progress }
end

function WorldGenerationCallHandler:update_progress(e)
   progress:update({
      progress = e.progress,
      complete = e.complete   
   })
end
   
return WorldGenerationCallHandler
