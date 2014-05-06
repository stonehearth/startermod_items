local PlantField = class()

function PlantField:__init()
end

function PlantField:initialize(properties, services)
   local info = properties.scenario_info
   local uri = info.entity_type
   local width = properties.size.width
   local height = properties.size.length

   for i=1, width do
      for j=1, height do
         services:place_entity(uri, i, j)
      end
   end

end

return PlantField
