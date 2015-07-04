local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Mob = _radiant.om.Mob
local Transform = _radiant.csg.Transform
local log = radiant.log.create_logger('physics')

local PhysicsService = class()

function PhysicsService:__init()
   self._stuckable_collision_types = {
      [Mob.TINY] = true,
      [Mob.HUMANOID] = true,
      [Mob.TITAN] = true,
      [Mob.CLUTTER] = true,
   }
end

function PhysicsService:initialize()
   self._sv = self.__saved_variables:get_data() 

   if not self._sv.in_motion_entities then
      self._sv.new_entities = {}
      self._sv.in_motion_entities = {}
   end
   self._dirty_tiles = {}

   self._gameloop_trace = radiant.events.listen(radiant, 'radiant:gameloop:start', self, self._update)   

   local entity_container = self:_get_root_entity_container()
   self._entity_container_trace = entity_container:trace_children('physics service')
      :on_added(function(id, entity)
            self._sv.new_entities[id] = entity
         end)

   self._dirty_tile_guard = _physics:add_notify_dirty_tile_fn(function(pt)
         self._dirty_tiles[pt:key_value()] = pt
      end)
end

function PhysicsService:_update()
   self:_process_new_entities()
   self:_update_dirty_tiles()
   self:_update_in_motion_entities()
end

function PhysicsService:_process_new_entities()
   for id, entity in pairs(self._sv.new_entities) do
      self:_unstick_entity(entity)
   end
   self._sv.new_entities = {}
end

function PhysicsService:_update_dirty_tiles()
   local dirty_tiles = self._dirty_tiles
   self._dirty_tiles = {}

   for _, pt in pairs(dirty_tiles) do
      log:debug('updating dirty tile %s', pt)
      _physics:for_each_entity_in_tile(pt, function(entity)
            if entity:get_id() ~= 1 then
               self:_unstick_entity(entity)
            end
         end)
   end
end

function PhysicsService:_unstick_entity(entity)
   if not self:_is_stuck(entity) then
      return
   end

   log:debug('unsticking %s', entity)

   local mob = entity:get_component('mob')
   local collision_type = mob:get_mob_collision_type()

   if collision_type == Mob.CLUTTER then
      log:debug('destroying %s', entity)
      radiant.entities.destroy_entity(entity)
      return
   end

   local current = mob:get_world_grid_location()
   local valid = _physics:get_standable_point(entity, current)

   if current.y < valid.y then
      self:_bump_to_standable_location(entity)
   elseif current.y > valid.y then
      self:_set_free_motion(entity)
   end
end

function PhysicsService:_is_stuck(entity)
   local mob = entity:get_component('mob')
   if not mob or mob:get_in_free_motion() or mob:get_ignore_gravity() then
      return false
   end

   local parent = mob:get_parent()
   if parent and parent:get_id() ~= 1 then
      -- only unstick entities that are children (not descendants) of the root entity
      return false
   end

   local current = mob:get_world_grid_location()
   if not current then
      return false
   end

   local collision_type = mob:get_mob_collision_type()

   if self._stuckable_collision_types[collision_type] then
      local valid = _physics:get_standable_point(entity, current)
      local result = current ~= valid
      return result
   end

   return false
end

function PhysicsService:_bump_to_standable_location(entity)
   local mob = entity:add_component('mob')
   local current = mob:get_world_location()
   local current_grid_location = current:to_closest_int()
   local candidates = {}
   local radius = 1

   -- prioritize bumps to the same elevation, then up, then down
   local get_score_penalty = function(current, candidate)
         local current_y = current.y
         local candidate_y = candidate.y

         if current_y == candidate_y then
            return 0.0
         end

         if current_y > candidate_y then
            return 0.5
         end

         -- current_y < candidate_y
         return 1.0
      end

   local get_candidate = function(location)
         local candidate = _physics:get_standable_point(entity, location)
         -- note that this uses the non-grid-aligned location, so it will favor
         -- some directions, which is what we want
         local distance = current:distance_to(candidate)
         local score_penalty = get_score_penalty(current, candidate)
         local score = distance + score_penalty
         local entry = {
            location = candidate,
            score = score
         }
         return entry
      end

   -- could be faster with an early exit or multi-radius search, but unstick doesn't run very often
   for j = -radius, radius do
      for i = -radius, radius do
         local direction = Point3(i, 0, j)
         local entry = get_candidate(current_grid_location + direction)
         table.insert(candidates, entry)
      end
   end

   table.sort(candidates, function(a, b)
         return a.score < b.score
      end)

   -- pick the candidate with the lowest score
   local selected = candidates[1].location

   log:debug('bumping %s to %s', entity, selected)
   mob:move_to(selected)
end

function PhysicsService:_set_free_motion(entity)
   log:debug('putting %s into free motion', entity)
   local mob = entity:add_component('mob')
   mob:set_in_free_motion(true)
   mob:set_velocity(Transform())
   self._sv.in_motion_entities[entity:get_id()] = entity
end

function PhysicsService:_update_in_motion_entities()
   for id, entity in pairs(self._sv.in_motion_entities) do
      if not self:_move_entity(entity) then
         self._sv.in_motion_entities[id] = nil
      end
   end
end

function PhysicsService:_move_entity(entity)
   local mob = entity:get_component('mob')
   if not mob then
      return false
   end
   
   if not mob:get_in_free_motion() then
      log:debug('taking %s out of free motion', entity)
      return false
   end

   -- Accleration due to gravity is 9.8 m/(s*s).  One block is one meter.
   -- You do the math (oh wait.  there isn't any! =)
   local acceleration = 9.8 / _radiant.sim.get_game_tick_interval();

   -- Update velocity.  Terminal velocity is currently 1-block per tick
   -- to make it really easy to figure out where the thing lands.
   local velocity = mob:get_velocity()
      
   log:debug('adding %.2f to %s current velocity %s', acceleration, entity, velocity)

   velocity.position.y = velocity.position.y - acceleration;
   velocity.position.y = math.max(velocity.position.y, -1.0);

   -- Update position
   local current = mob:get_transform()
   local nxt = Transform()
   nxt.position = current.position + velocity.position
   nxt.orientation = current.orientation

   -- when testing to see if we're blocked, make sure we look at the right point.
   -- `is_blocked` will round to the closest int, so if we're at (1, -0.3, 1), it
   -- will actually test the point (1, 0, 1) when we wanted (1, -1, 1) !!
   local test_position = nxt.position - Point3(0, 0.5, 0)
   
   -- If our next position is blocked, fall to the bottom of the current
   -- brick and clear the free motion flag.
   if _physics:is_blocked(entity, test_position) then
      log:debug('next position %s is blocked.  leaving free motion', test_position)

      velocity.position = Point3.zero
      nxt.position.y = math.floor(current.position.y)
      mob:set_in_free_motion(false)
   end   

   -- Update our actual velocity and position.  Return false if we left
   -- the free motion state to get the task pruned
   mob:set_velocity(velocity);
   mob:set_transform(nxt);

   return mob:get_in_free_motion()
end

function PhysicsService:_get_root_entity_container()
   return radiant._root_entity:add_component('entity_container')
end

return PhysicsService
