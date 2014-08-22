local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Rect2 = _radiant.csg.Rect2
local Cube3 = _radiant.csg.Cube3
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('trapping')

local TrappingGroundsComponent = class()

local function getXZCubeAroundPoint(location, radius)
   local cube = Cube3(
      Point3(location.x-radius,   location.y,   location.z-radius),
      Point3(location.x+radius+1, location.y+1, location.z+radius+1)
   )
   return cube
end

function has_ai(entity)
   return entity and entity:is_valid() and entity:get_component('stonehearth:ai') ~= nil
end

function TrappingGroundsComponent:__init()
end

function TrappingGroundsComponent:initialize(entity, json)
   self._entity = entity

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._sv.traps = {}
      self._sv.num_traps = 0
      self._sv.max_traps = json.max_traps or 4
      self._sv.min_distance_between_traps = json.min_distance_between_traps or 16
      self._sv.spawn_interval_min = stonehearth.calendar:duration_string_to_seconds(json.spawn_interval_min)
      self._sv.spawn_interval_max = stonehearth.calendar:duration_string_to_seconds(json.spawn_interval_max)
      self._sv.check_traps_interval = stonehearth.calendar:duration_string_to_seconds(json.check_traps_interval)
      self._sv.initialized = true
      self.__saved_variables:mark_changed()

      -- radiant.events.listen_once(self._entity, 'radiant:entity:post_create',
      --    function ()
      --       self:start_tasks()
      --    end
      -- )
   else
      self:_restore()
   end
end

function TrappingGroundsComponent:_restore()
   for id, trap in pairs(self._sv.traps) do
      self:_listen_to_destroy_trap(trap)
   end

   if self._sv.next_check_trap_time then
      local duration = self:_time_to_duration(self._sv.next_check_trap_time)
      self:_start_check_trap_timer(duration)
   end

   if self._sv.next_spawn_time then
      local duration = self:_time_to_duration(self._sv.next_spawn_time)
      self:_start_spawn_timer(duration)
   end
end

function TrappingGroundsComponent:destroy()
   self:_destroy_survey_task()
   self:_destroy_set_trap_task()
   self:_destroy_check_trap_task()

   for id, trap in pairs(self._sv.traps) do
      local bait_trap_component = trap:add_component('stonehearth:bait_trap')
      bait_trap_component:release()
      radiant.entities.destroy_entity(trap)
   end
end

function TrappingGroundsComponent:get_size()
   return self._sv.size
end

function TrappingGroundsComponent:set_size(x, z)
   -- resizing with active traps not yet supported
   assert(self._sv.num_traps == 0)

   self._sv.size = { x = x, z = z}
   self.__saved_variables:mark_changed()
end

function TrappingGroundsComponent:start_tasks()
   if self._sv.num_traps == 0 then
      -- preferred self._survey_task:wait() instead of using callback, but it doesn't work here
      self:_create_survey_task(
         function()
            self:_create_set_trap_task()
         end
      )
   else
      self:_create_check_trap_task()
   end
end

function TrappingGroundsComponent:get_traps()
   return self._sv.traps
end

function TrappingGroundsComponent:get_armed_traps()
   local armed_traps = {}

   for id, trap in pairs(self._sv.traps) do
      -- if we store trap components instead of traps, we could be polymorphic and support other types of traps
      local trap_component = trap:add_component('stonehearth:bait_trap')
      if trap_component:is_armed() then
         armed_traps[id] = trap
      end
   end

   return armed_traps
end

function TrappingGroundsComponent:max_traps()
   return self._sv.max_traps
end

function TrappingGroundsComponent:add_trap(trap)
   local id = trap:get_id()

   if not self._sv.traps[id] then
      self:_listen_to_destroy_trap(trap)

      self._sv.traps[id] = trap
      self._sv.num_traps = self._sv.num_traps + 1
      self.__saved_variables:mark_changed()
   end

   if self._sv.num_traps == 1 then
      -- start timer when first trap is added
      self:_start_check_trap_timer(self._sv.check_traps_interval)
      local duration = self:_get_spawn_duration()
      self:_start_spawn_timer(duration)
   end
end

function TrappingGroundsComponent:remove_trap(id)
   if self._sv.traps[id] then
      local trap = radiant.entities.get_entity(id)
      if trap:is_valid() then
         self:_unlisten_to_destroy_trap(trap)
      end

      self._sv.traps[id] = nil
      self._sv.num_traps = self._sv.num_traps - 1
      self.__saved_variables:mark_changed()
   end

   if self._sv.num_traps == 0 then
      -- when all traps have been harvested, start setting them again
      self:start_tasks()
   end
end

function TrappingGroundsComponent:_listen_to_destroy_trap(trap)
   radiant.events.listen(trap, 'radiant:entity:pre_destroy', self, self._on_destroy_trap)
end

function TrappingGroundsComponent:_unlisten_to_destroy_trap(trap)
   radiant.events.unlisten(trap, 'radiant:entity:pre_destroy', self, self._on_destroy_trap)
end

function TrappingGroundsComponent:_on_destroy_trap(args)
   self:remove_trap(args.entity_id)
end

function TrappingGroundsComponent:_pick_next_trap_location()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local size = self._sv.size
   local tries = 5

   if size.x < 3 or size.z < 3 then
      -- no space for trap
      return nil
   end

   -- setting n traps takes O(n^2) time, but n is tiny
   -- use services.server.world_generation.perturbation_grid to make this O(n) instead
   -- currently not worth the added code complexity
   for i = 1, tries do
      -- offset is 0 to size-1, with a 1 voxel margin to avoid edges
      local dx = rng:get_int(1, size.x-2)
      local dz = rng:get_int(1, size.z-2)
      local location = Point3(origin.x + dx, origin.y, origin.z + dz)

      if self:_is_valid_trap_location(location, 1) then
         return location
      end
   end

   return nil
end

function TrappingGroundsComponent:_is_valid_trap_location(location)
   -- this iteration can be avoided by using a perturbation grid
   for id, trap in pairs(self._sv.traps) do
      if radiant.entities.distance_between(trap, location) < self._sv.min_distance_between_traps then
         return false
      end
   end

   -- make sure 3x3 cube around location is clear
   return self:_is_empty_location(location, 1)
end

function TrappingGroundsComponent:_is_empty_location(location, radius)
   local cube = getXZCubeAroundPoint(location, radius)
   local entities = radiant.terrain.get_entities_in_cube(cube)

   -- remove the trapping grounds from the set
   entities[self._entity:get_id()] = nil

   local empty = not next(entities)
   return empty
end

function TrappingGroundsComponent:_destroy_survey_task()
   if self._survey_task then
      self._survey_task:destroy()
      self._survey_task = nil
   end
end

function TrappingGroundsComponent:_create_survey_task(completed_callback)
   local town = stonehearth.town:get_town(self._entity)

   if self._survey_task or not town then
      return
   end

   self._survey_task = town:create_task_for_group('stonehearth:task_group:trapping',
                                                  'stonehearth:trapping:survey_trapping_grounds',
                                                  {
                                                     trapping_grounds = self._entity
                                                  })
      :set_source(self._entity)
      :set_name('survey trapping grounds')
      :set_priority(stonehearth.constants.priorities.trapping.SURVEY_TRAPPING_GROUNDS)
      :once()
      :notify_completed(
         function ()
            self._survey_task = nil
            if completed_callback then
               completed_callback()
            end
         end
      )
      :start()
end

function TrappingGroundsComponent:_destroy_set_trap_task()
   if self._set_trap_task then
      self._set_trap_task:destroy()
      self._set_trap_task = nil
   end
end

function TrappingGroundsComponent:_create_set_trap_task()
   local trap_uri = 'stonehearth:trapper:snare_trap'
   local town = stonehearth.town:get_town(self._entity)

   if self._set_trap_task or self._sv.num_traps >= self._sv.max_traps or not town then
      return
   end

   local location = self:_pick_next_trap_location()
   if not location then
      return
   end

   self._set_trap_task = town:create_task_for_group('stonehearth:task_group:trapping',
                                                    'stonehearth:trapping:set_bait_trap',
                                                    {
                                                       location = location,
                                                       trap_uri = trap_uri,
                                                       trapping_grounds = self._entity
                                                    })
      :set_source(self._entity)
      :set_name('set trap task')
      :set_priority(stonehearth.constants.priorities.trapping.SET_TRAPS)
      :once()
      :notify_completed(
         function ()
            self._set_trap_task = nil
            self:_create_set_trap_task() -- keep setting traps serially until done
         end
      )
      :start()
end

function TrappingGroundsComponent:_destroy_check_trap_task()
   if self._check_trap_task then
      self._check_trap_task:destroy()
      self._check_trap_task = nil
   end
end

function TrappingGroundsComponent:_create_check_trap_task()
   local town = stonehearth.town:get_town(self._entity)

   if self._check_trap_task or self._sv.num_traps == 0 or not town then
      -- exit if we're already checking traps or we have nothing to do
      return
   end

   self._check_trap_task = town:create_task_for_group('stonehearth:task_group:trapping',
                                                      'stonehearth:trapping:check_bait_trap',
                                                      {
                                                         trapping_grounds = self._entity
                                                      })
      :set_source(self._entity)
      :set_name('check trap task')
      :set_priority(stonehearth.constants.priorities.trapping.CHECK_TRAPS)
      :once()
      :notify_completed(
         function ()
            self._check_trap_task = nil
            self:_create_check_trap_task() -- keep checking traps serially until done
         end
      )
      :start()
end

function TrappingGroundsComponent:_start_check_trap_timer(duration)
   self:_stop_check_trap_timer()

   self._check_trap_timer = stonehearth.calendar:set_timer(duration,
      function()
         self:_stop_check_trap_timer()
         self:_create_check_trap_task()
      end
   )

   self._sv.next_check_trap_time = self._check_trap_timer:get_expire_time()
end

function TrappingGroundsComponent:_stop_check_trap_timer()
   if self._check_trap_timer then
      self._check_trap_timer:destroy()
      self._check_trap_timer = nil
   end

   self._sv.next_check_trap_time = nil
end

function TrappingGroundsComponent:_start_spawn_timer(duration)
   self:_stop_spawn_timer()

   self._spawn_timer = stonehearth.calendar:set_timer(duration,
      function ()
         self:_stop_spawn_timer()
         self:_try_spawn()
      end
   )

   self._sv.next_spawn_time = self._spawn_timer:get_expire_time()
end

function TrappingGroundsComponent:_stop_spawn_timer()
   if self._spawn_timer then
      self._spawn_timer:destroy()
      self._spawn_timer = nil
   end

   self._sv.next_spawn_time = nil
end

function TrappingGroundsComponent:_try_spawn()
   -- must be within the interaction sensor range of the trap
   local max_spawn_distance = 15
   -- won't spawn if threat within this distance of trap or spawn location
   -- TODO: refactor this distance with the avoid_threatening_entities_observer
   local threat_distance = 16

   local armed_traps = self:get_armed_traps()
   local trap = self:_choose_random_trap(armed_traps)

   -- don't retry if there are threats detected. you lose this spawn, do not pass go
   -- place trapping grounds away from trafficed areas if you want to catch things
   if trap and self:_location_is_clear_of_threats(trap, threat_distance) then
      local critter_uri = self:_choose_spawned_critter_type()
      local critter = self:_create_critter(critter_uri)

      local spawn_location = self:_get_spawn_location(trap, max_spawn_distance, critter)

      if spawn_location and self:_location_is_clear_of_threats(spawn_location, threat_distance) then
         radiant.terrain.place_entity(critter, spawn_location)
      else
         radiant.entities.destroy_entity(critter)
      end
   end

   local duration = self:_get_spawn_duration()
   self:_start_spawn_timer(duration)
end

-- takes an Point3 or Entity
function TrappingGroundsComponent:_location_is_clear_of_threats(location, threat_distance)
   if radiant.entities.is_entity(location) then
      location = radiant.entities.get_world_grid_location(location)
   end

   local cube = getXZCubeAroundPoint(location, threat_distance)
   local entities = radiant.terrain.get_entities_in_cube(cube)

   for id, other_entity in pairs(entities) do
      if self:_is_threatening_to_critter(other_entity) then
         return false
      end
   end

   return true
end

function TrappingGroundsComponent:_is_threatening_to_critter(other_entity)
   if not has_ai(other_entity) then
      return false
   end

   local other_faction = radiant.entities.get_faction(other_entity)
   return other_faction ~= 'critter'
end

function TrappingGroundsComponent:_create_critter(uri)
   local critter = radiant.entities.create_entity(uri)
   radiant.entities.set_player_id(critter, 'game_master')

   local despawn_ai = self:_inject_despawn_ai(critter)

   local trapped_trace = radiant.events.listen_once(critter, 'stonehearth:trapped',
      function()
         despawn_ai:destroy()
      end
   )

   local destroy_trace = radiant.events.listen_once(critter, 'stonehearth:pre-destroy',
      function()
         trapped_trace:destroy()
      end
   )

   return critter
end

function TrappingGroundsComponent:_inject_despawn_ai(entity)
   return stonehearth.ai:inject_ai(entity, {
      actions = {
         "stonehearth:actions:trapping:despawn"
      }
   })
end

function TrappingGroundsComponent:_get_spawn_location(trap, distance, spawned_entity)
   local proposed_spawn_location = self:_get_proposed_spawn_location(trap, distance)
   local actual_spawn_location = self:_get_actual_spawn_location(trap, proposed_spawn_location, spawned_entity)
   return actual_spawn_location
end

function TrappingGroundsComponent:_get_proposed_spawn_location(trap, distance)
   -- pick a random location that is spawn_distance away from the trap
   local trap_location = radiant.entities.get_world_location(trap)
   local vector = radiant.math.random_xz_unit_vector(rng)
   vector:scale(distance)
   local spawn_location = trap_location + vector

   -- TODO: should be able to call to_closest_int here
   local x = radiant.math.round(spawn_location.x)
   local y = radiant.math.round(spawn_location.y)
   local z = radiant.math.round(spawn_location.z)
   local spawn_grid_location = Point3(x, y, z)

   return spawn_grid_location
end

function TrappingGroundsComponent:_get_actual_spawn_location(trap, proposed_spawn_location, spawned_entity)
   local trap_location = radiant.entities.get_world_grid_location(trap)
   local actual_spawn_location = radiant.terrain.get_direct_path_end_point(trap_location, proposed_spawn_location, spawned_entity)
   return actual_spawn_location
end

-- TODO: make this a function of the location of the traps
function TrappingGroundsComponent:_choose_spawned_critter_type()
   local critter_types = {
      'stonehearth:rabbit',
      'stonehearth:racoon',
      'stonehearth:red_fox',
      'stonehearth:squirrel',
   }
   local roll = rng:get_int(1, #critter_types)
   return critter_types[roll]
end

function TrappingGroundsComponent:_choose_random_trap(traps)
   if not next(traps) then
      return nil
   end

   local trap_array = {}
   for _, trap in pairs(traps) do
      table.insert(trap_array, trap)
   end

   local roll = rng:get_int(1, #trap_array)
   return trap_array[roll]
end

function TrappingGroundsComponent:_get_spawn_duration()
   local duration = rng:get_real(self._sv.spawn_interval_min, self._sv.spawn_interval_max)
   return duration
end

function TrappingGroundsComponent:_time_to_duration(time)
   local duration = time - stonehearth.calendar:get_elapsed_time()
   duration = math.max(duration, 1) -- timer doesn't like 0 durations?
   return duration
end

return TrappingGroundsComponent
