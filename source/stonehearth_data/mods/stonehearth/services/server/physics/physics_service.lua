
local PhysicsService = class()
local Mob = _radiant.om.Mob

local Point3 = _radiant.csg.Point3
local Transform = _radiant.csg.Transform
local Quaternion = _radiant.csg.Quaternion

local log = radiant.log.create_logger('physics')

function PhysicsService:initialize()
   self._sv = self.__saved_variables:get_data() 

   if not self._sv.in_motion_entities then
      self._sv.new_entities = {}
      self._sv.in_motion_entities = {}
   end
   self._dirty_tiles = {}

   radiant.events.listen(radiant, 'radiant:gameloop:start', self, self._update)   
   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         self._sv.new_entities[entity:get_id()] = entity
      end)
   self._guard = _physics:add_notify_dirty_tile_fn(function(pt)
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
      _physics:for_each_entity_in_tile(pt, function(entity)
            if entity:get_id() ~= 1 then
               self:_unstick_entity(entity)
            end
         end)
   end
end

function PhysicsService:_unstick_entity(entity)
   local mob = entity:get_component('mob')
   if not mob or mob:get_in_free_motion() or mob:get_ignore_gravity() then
      return
   end
   local current = radiant.entities.get_world_location(entity)
   if not current then
      return
   end
   local collision_type = mob:get_mob_collision_type()
   if collision_type == Mob.TINY or
      collision_type == Mob.HUMANOID or
      collision_type == Mob.TITAN then
      local valid = _physics:get_standable_point(entity, current)
      if current.y < valid.y then
         local dy = valid.y - current.y
         local location = mob:get_location() + Point3(0, dy, 0)
         log:debug('popping %s up to %s', entity, location)
         mob:move_to(location)
      elseif current.y > valid.y then
         log:debug('putting %s into free motion', entity)
         mob:set_in_free_motion(true)
         mob:set_velocity(Transform())
         self._sv.in_motion_entities[entity:get_id()] = entity
      end

   elseif collision_type == Mob.CLUTTER then
      radiant.entities.destroy_entity(entity)
   end
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

   -- If our next position is blocked, fall to the bottom of the current
   -- brick and clear the free motion flag.
   if _physics:is_blocked(entity, nxt.position) then
      log:debug('next position %s is blocked.  leaving free motion', nxt.position)

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

return PhysicsService
