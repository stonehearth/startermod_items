local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('swimming')

SwimmingService = class()

function SwimmingService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
   end

   self._location_traces = {}
   self._swimming_state = {}
   self:_create_mob_height_table()
   self:_create_entity_traces()
end

function SwimmingService:destroy()
end

function SwimmingService:_create_entity_traces()
   local entity_container = self:_get_root_entity_container()

   self._entity_container_trace = entity_container:trace_children('swimming')
      :on_added(function(id, entity)
            if not entity:get_component('stonehearth:ai') then
               -- doesn't walk / swim
               return
            end
            self._swimming_state[id] = false
            self:_trace_entity(id, entity)
         end)
      :on_removed(function(id)
            self:_destroy_entity_trace(id)
         end)
      :push_object_state()
end

function SwimmingService:_trace_entity(id, entity)
   if self._location_traces[id] then
      -- trace already exists
      return
   end

   self._location_traces[id] = radiant.entities.trace_grid_location(entity, 'swimming')
      :on_changed(function()
            self:_update(id, entity)
         end)
      :push_object_state()
end

function SwimmingService:_destroy_entity_trace(id)
   local trace = self._location_traces[id]
   if trace then
      trace:destroy()
      self._location_traces[id] = nil
   end
end

function SwimmingService:_destroy_entity_traces()
   if self._entity_container_trace then
      self._entity_container_trace:destroy()
      self._entity_container_trace = nil
   end

   for id in pairs(self._location_traces) do
      self:_destroy_entity_trace(id)
   end
end

function SwimmingService:_update(id, entity)
   local swimming = self:_is_swimming(entity)

   if swimming ~= self._swimming_state[id] then
      self._swimming_state[id] = swimming

      if swimming then
         radiant.entities.set_posture(entity, 'stonehearth:swimming')
         radiant.entities.add_buff(entity, 'stonehearth:buffs:swimming')
      else
         radiant.entities.unset_posture(entity, 'stonehearth:swimming')
         radiant.entities.remove_buff(entity, 'stonehearth:buffs:swimming')
      end
   end
end

function SwimmingService:_is_swimming(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   local mob_collision_type = entity:add_component('mob'):get_mob_collision_type()
   local entity_height = self._mob_heights[mob_collision_type]
   if not entity_height then
      log:warning('unsupported mob_collision_type for swimming')
      return false
   end

   local cube = Cube3(location)
   cube.max.y = cube.min.y + entity_height
   local intersected_entities = radiant.terrain.get_entities_in_cube(cube)
   local swimming = false

   for id, entity in pairs(intersected_entities) do
      local water_component = entity:get_component('stonehearth:water')
      if water_component then
         local water_level = water_component:get_water_level()
         local swim_level = location.y + entity_height * 0.5
         if water_level > swim_level then
            swimming = true
            break
         end
      end
   end

   return swimming
end

function SwimmingService:_create_mob_height_table()
   self._mob_heights = {
      [_radiant.om.Mob.NONE] = 1,
      [_radiant.om.Mob.TINY] = 1,
      [_radiant.om.Mob.HUMANOID] = 3
   }
end

function SwimmingService:_get_root_entity_container()
   return radiant._root_entity:add_component('entity_container')
end

return SwimmingService
