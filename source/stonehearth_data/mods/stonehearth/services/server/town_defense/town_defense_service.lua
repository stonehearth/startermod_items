local PatrollableObject = require 'services.server.town_defense.patrollable_object'
local log = radiant.log.create_logger('town_defense')

TownDefense = class()

-- Conventions adopted in this class:
--   entity: a unit (like a footman)
--   object: everything else, typically something that can be patrolled

function TownDefense:__init()
   -- traces for change of ownership of a patrollable object
   self._player_id_traces = {}
end

function TownDefense:initialize()
   self._sv = self.__saved_variables:get_data()

   -- push_object_state is called for us by trace_world_entities
   -- hold on to the trace so we have a refcount for it
   self._world_objects_trace = radiant.terrain.trace_world_entities('town defense service', 
      function (id, object)
         self:_on_object_added(object)
      end,
      function (id)
         self:_on_object_removed(id)
      end
   )

   if not self._sv.initialized then
      -- holds all the patrollable objects in the world organized by player_id
      self._sv.patrollable_objects = {}

      -- maps patrollable objects to their owning player. used to provide O(1) destroy.
      self._sv.object_to_player_map = {}

      self._sv.initialized = true
      self._world_objects_trace:push_object_state()
   else
      for player_id, player_patrollable_objects in pairs(self._sv.patrollable_objects) do
         for object_id, patrollable_object in pairs(player_patrollable_objects) do
            local object = patrollable_object:get_object()
            -- restore traces
            self:_add_player_id_trace(object)
         end
         -- notify any existing entities of available patrol routes
         self:_trigger_patrol_route_available(player_id)
      end
   end
end

function TownDefense:destroy()
end

function TownDefense:get_patrol_route(entity)
   local patrollable_object = self:_get_object_to_patrol(entity)
   return patrollable_object
end

-- returns true if all ok
-- returns false if patrol route is now invalid
function TownDefense:mark_patrol_started(entity, patrollable_object)
   local acquired = patrollable_object:acquire_lease(entity)
   return acquired
end

-- TODO: automatically release leases after a certain time
function TownDefense:mark_patrol_completed(entity, patrollable_object)
   patrollable_object:mark_visited()

   local released = patrollable_object:release_lease(entity)
   if released then
      local player_id = radiant.entities.get_player_id(patrollable_object:get_object())
      self:_trigger_patrol_route_available(player_id)
   else
      log:error('unable to release lease on object %s by %s', patrollable_object:get_object(), entity)
   end
end

-- returns the patrollable object that is the best fit for this entity
function TownDefense:_get_object_to_patrol(entity)
   local location = radiant.entities.get_world_location(entity)
   local unleased_objects = self:_get_unleased_objects(entity)
   local ordered_objects = {}

   if next(unleased_objects) == nil then
      return nil
   end

   -- score each object based on the benefit/cost ratio of patrolling it
   for object_id, patrollable_object in pairs(unleased_objects) do
      local score = self:_calculate_patrol_score(location, patrollable_object)
      local sort_info = { score = score, patrollable_object = patrollable_object }
      table.insert(ordered_objects, sort_info)
   end

   -- order the objects by highest score
   table.sort(ordered_objects,
      function (a, b)
         return a.score > b.score
      end
   )

   -- return the object with the highest score
   local patrollable_object = ordered_objects[1].patrollable_object
   return patrollable_object
end

-- estimate the benefit/cost ratio of patrolling an object
-- cost is the distance to travel to the object
-- benefit is the amount of time since we last patrolled it
function TownDefense:_calculate_patrol_score(start_location, patrollable_object)
   local distance = start_location:distance_to(patrollable_object:get_centroid())
   -- add the minimum cost to patrol the perimeter of a 1x1 object at distance 2
   -- this keeps really close objects from getting an overinflated score because there is
   -- a nonzero cost to patrolling them 
   distance = distance + 16

   local time = radiant.gamestate.now() - patrollable_object:get_last_patrol_time()
   -- if time is zero, then let distance be the tiebreaker
   if time < 1 then
      time = 1
   end
   
   local score = time / distance
   return score
end

function TownDefense:_on_object_added(object)
   if self:_is_patrollable(object) then
      self:_add_to_patrol_list(object)
   end
end

function TownDefense:_on_object_removed(object_id)
   self:_remove_from_patrol_list(object_id)
end

function TownDefense:_add_player_id_trace(object)
   local object_id = object:get_id()
   local player_id_trace = object:add_component('unit_info'):trace_player_id('town defense')
      :on_changed(
         function ()
            self:_on_player_id_changed(object)
         end
      )

   -- keep a refcount for the trace so it stays around
   self._player_id_traces[object_id] = player_id_trace
end

function TownDefense:_remove_player_id_trace(object_id)
   self._player_id_traces[object_id] = nil
end

function TownDefense:_on_player_id_changed(object)
   self:_remove_from_patrol_list(object:get_id())
   self:_add_to_patrol_list(object)
end

function TownDefense:_is_patrollable(object)
   if object == nil or not object:is_valid() then
      return false
   end

   local entity_data = radiant.entities.get_entity_data(object, 'stonehearth:town_defense')
   return entity_data and entity_data.auto_patrol == true
end

function TownDefense:_add_to_patrol_list(object)
   local patrollable_object = radiant.create_controller('stonehearth:patrollable_object', object)
   local object_id = object:get_id()
   local player_id = radiant.entities.get_player_id(object)

   if player_id then
      local player_patrollable_objects = self:_get_patrollable_objects(player_id)

      player_patrollable_objects[object_id] = patrollable_object
      self._sv.object_to_player_map[object_id] = player_id

      self:_trigger_patrol_route_available(player_id)
   end

   -- trace all objects that are patrollable in case their ownership changes
   self:_add_player_id_trace(object)

   self.__saved_variables:mark_changed()
end

function TownDefense:_remove_from_patrol_list(object_id)
   local player_id = self._sv.object_to_player_map[object_id]
   self._sv.object_to_player_map[object_id] = nil

   if player_id then
      local player_patrollable_objects = self:_get_patrollable_objects(player_id)

      if player_patrollable_objects[object_id] then
         -- optionally cancel existing patrol routes here
         player_patrollable_objects[object_id] = nil
      end
   end

   self:_remove_player_id_trace(object_id)

   self.__saved_variables:mark_changed()
end

function TownDefense:_get_patrollable_objects(player_id)
   local player_patrollable_objects = self._sv.patrollable_objects[player_id]

   if not player_patrollable_objects then
      player_patrollable_objects = {}
      self._sv.patrollable_objects[player_id] = player_patrollable_objects
   end

   return player_patrollable_objects
end

function TownDefense:_get_unleased_objects(entity)
   local player_id = radiant.entities.get_player_id(entity)
   local player_patrollable_objects = self:_get_patrollable_objects(player_id)
   local unleased_objects = {}

   for object_id, patrollable_object in pairs(player_patrollable_objects) do
      if patrollable_object:can_acquire_lease(entity) then
         unleased_objects[object_id] = patrollable_object
      end
   end

   return unleased_objects
end

function TownDefense:_trigger_patrol_route_available(player_id)
   radiant.events.trigger_async(stonehearth.town_defense, 'stonehearth:patrol_route_available', { player_id = player_id })
end

return TownDefense
