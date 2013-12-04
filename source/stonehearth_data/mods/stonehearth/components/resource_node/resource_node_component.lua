local ResourceNodeComponent = class()

function ResourceNodeComponent:__init(entity)
   self._entity = entity
   self._durability = 1
end
   
function ResourceNodeComponent:extend(json)
   -- not really...
   if json.durability then
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      -- xXXXXXXXXXXXXXXXXXXXXXXX DO NOT SHIP THIS
      --self._durability = json.durability
   end
   if json.resource then
      self._resource = json.resource
   end
end

function ResourceNodeComponent:spawn_resource(collect_location)
   if self._resource then
      local item = radiant.entities.create_entity(self._resource)
      collect_location.x = collect_location.x + math.random(-3, 3)
      collect_location.z = collect_location.z + math.random(-3, 3)      
      radiant.terrain.place_entity(item, collect_location)
      
      self._durability = self._durability - 1
      if self._durability <= 0 then
         radiant.entities.destroy_entity(self._entity)
      end
   end
end

function ResourceNodeComponent:get_harvest_locations(collect_location)
   local result = PointList()
   local origin = self._entity:get_component('mob'):get_world_grid_location()
   
   local r = 3
   for i = -r, r do
   result:insert(Point3(origin.x + i, origin.y, origin.z + r))
      result:insert(Point3(origin.x + i, origin.y, origin.z - r))
      result:insert(Point3(origin.x + r, origin.y, origin.z + i))
      result:insert(Point3(origin.x - r, origin.y, origin.z + i))
   end
   return result
end

return ResourceNodeComponent
