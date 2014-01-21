local WorldGenerationCallHandler = class()
local log = radiant.log.create_logger('world_generation')

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
   local args = {}

   args.progress = e.progress
   
   if args.progress == 100 then
      args.width = e.width
      args.height = e.height
      args.elevation_map = e.elevation_map:clone_to_nested_arrays()
      args.forest_map = e.forest_map:clone_to_nested_arrays()
   end

   progress:update(args)
end
   
return WorldGenerationCallHandler
