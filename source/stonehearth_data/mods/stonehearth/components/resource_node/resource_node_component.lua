local ResourceNodeComponent = class()

function ResourceNodeComponent:initialize(entity, json)
   self._entity = entity
   self._harvester_effect = json.harvester_effect
   self._task_group_name = json.task_group_name
   self._description = json.description
   self._harvest_overlay_effect = json.harvest_overlay_effect
   self._harvest_tool = json.harvest_tool

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._sv.durability = json.durability or 1
      self._sv.harvestable = true
      self._sv.initialized = true
      self._sv.resource = json.resource
   end
end

function ResourceNodeComponent:is_harvestable()
   return self._sv.durability > 0
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
   return self._harvester_effect or 'fiddle'
end

function ResourceNodeComponent:get_task_group_name()
   return self._task_group_name
end

function ResourceNodeComponent:get_description()
   return self._description
end

-- If the resource is nil, the object will still eventually disappear
function ResourceNodeComponent:spawn_resource(owner, collect_location)
   if self._sv.resource then
      local item = radiant.entities.create_entity(self._sv.resource)
      local pt = radiant.terrain.find_placement_point(collect_location, 0, 4)
      radiant.terrain.place_entity(item, pt)

      -- add it to the inventory of the owner
      stonehearth.inventory:get_inventory(owner)
                              :add_item(item)
   end

   self._sv.durability = self._sv.durability - 1

   if self._sv.durability <= 0 then
      radiant.entities.kill_entity(self._entity)
   end

   self.__saved_variables:mark_changed()
end

return ResourceNodeComponent
