local log = radiant.log.create_logger('visibility')

local TerrainCallHandler = class()

function TerrainCallHandler:get_visibility_regions(session, request)
   local visible_region = stonehearth.terrain:get_visible_region(session.faction)
   local explored_region = stonehearth.terrain:get_explored_region(session.faction)

   return {
      -- these boxed regions will serialize into a uri handle
      visible_region_uri = visible_region,
      explored_region_uri = explored_region
   }
end

return TerrainCallHandler
