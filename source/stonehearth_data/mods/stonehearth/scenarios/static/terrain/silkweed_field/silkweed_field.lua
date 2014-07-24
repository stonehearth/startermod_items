local rng = _radiant.csg.get_default_rng()
local PlantField = class()

function PlantField:__init()
end

function PlantField:initialize(properties, services)
   local info = properties.scenario_info
   local width = properties.size.width
   local height = properties.size.length
   local uri = info.entity_type
   local density = info.density

   for i=1, width do
      for j=1, height do
         if not density or rng:get_real(0, 1) < density then
            services:place_entity(uri, i, j)
         end
      end
   end

end

return PlantField
