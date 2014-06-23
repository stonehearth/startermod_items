local ResourceNodeComponent = class()

function ResourceNodeComponent:initialize(entity, json)
   self._entity = entity
   self._durability = json.durability or 1
   self._harvester_effect = json.harvester_effect
   self._task_group_name = json.task_group_name
   self._description = json.description
   self._harvest_overlay_effect = json.harvest_overlay_effect
   self._harvest_tool = json.harvest_tool

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.resource = json.resource
   end

end

-- Update the resource spawned by this entity
function ResourceNodeComponent:set_resource(resource)
   self._sv.resource = resource
end

function ResourceNodeComponent:get_harvest_tool()
   return self._harvest_tool
end

function ResourceNodeComponent:get_harvest_overlay_effect()
   return self._harvest_overlay_effect
end

function ResourceNodeComponent:get_harvester_effect()
   return self._harvester_effect
end

function ResourceNodeComponent:get_task_group_name()
   return self._task_group_name
end

function ResourceNodeComponent:get_description()
   return self._description
end

-- If the resource is nil, the object will still eventually disappear
function ResourceNodeComponent:spawn_resource(collect_location)
   if self._sv.resource then
      local item = radiant.entities.create_entity(self._sv.resource)
      collect_location.x = collect_location.x + math.random(-3, 3)
      collect_location.z = collect_location.z + math.random(-3, 3)      
      radiant.terrain.place_entity(item, collect_location)
   end
   self._durability = self._durability - 1
   if self._durability <= 0 then
      radiant.entities.destroy_entity(self._entity)
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
