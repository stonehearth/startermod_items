--[[
   Tell person to come and admire the fire
   This is an idle action, so it only happens when people have worked off their queue,
   but it always happens, people naturally gravitate towards moving, lit things.
   TODO: rename to "find fireside seat or something"
]]
local calendar = stonehearth.calendar

local AdmireFire = class()

AdmireFire.name = 'admire fire'
AdmireFire.does = 'stonehearth:idle'
AdmireFire.version = 1
AdmireFire.priority = 0

function AdmireFire:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._firepit_seat = nil
   self._path_to_fire = nil
   self._should_look_for_fire = false

   self._standing_fire_effects = radiant.entities.get_entity_data(entity, "stonehearth:idle_fire_effects")

   radiant.events.listen(calendar, 'stonehearth:sunrise', self, self.on_sunrise)

   --People look for lit firepits after dark.
   self._known_firepit_callbacks = {}
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end
   self._promise = radiant.terrain.trace_world_entities('firepit tracker', added_cb, removed_cb)

   self._time_constants = calendar.get_constants()
end

--- Trace all fire sources. Whenever one is added, trace its properties.
-- When a fire is lit and it's nighttime, tell people to look for fires.
-- TODO: how to release a trace?
-- TODO: Bug: I suspect people will not know how to look for fires that appeared
-- before they were added to the world.
function AdmireFire:_on_entity_add(id, entity)
   local firepit_component = entity:get_component('stonehearth:firepit')
   if firepit_component and
      radiant.entities.get_faction(entity) == radiant.entities.get_faction(self._entity) and 
      not self._known_firepit_callbacks[id] then

      local promise = firepit_component:get_data_store():trace_data('follow firepit data')
      self._known_firepit_callbacks[id] = promise
      promise:on_changed(function()
         if firepit_component:is_lit() then
            self:_should_find_fire()
         end
      end)
   end
end


function AdmireFire:_on_entity_remove(id)
   if self._known_firepit_callbacks[id] then
      self._known_firepit_callbacks[id] = nil
   end
end

---Whenever there is a lit fire, if idle, go to it and hang out
function AdmireFire:_should_find_fire()
   local curr_time = calendar.get_time_and_date()
   if curr_time.hour >= self._time_constants.event_times.sunset or
      curr_time.hour < self._time_constants.event_times.sunrise then
      self._should_look_for_fire = true
      if not self._pathfinder then
         self:_start_looking_for_fire()
      end
   end
end

--- At dawn, stop looking, release all seats, unset postures
function AdmireFire:on_sunrise(e)
   self._should_look_for_fire = false
   self:_clear_variables()
end

--- Given an item, determine if it's an unoccupied seat by a lit firepit
-- @param item The item to evaluate
-- @returns True if all conditions are met, false otherwise
function AdmireFire:_is_seat_by_lit_firepit(item)
   local lease_component = item:get_component('stonehearth:lease_component')
   if lease_component and lease_component:can_acquire_lease(self._entity) then

      local lit_fire = false
      local center_of_attention_spot_component = item:get_component('stonehearth:center_of_attention_spot')
      if center_of_attention_spot_component then
         local center_of_attention = center_of_attention_spot_component:get_center_of_attention()
         if center_of_attention then
            local firepit_component = center_of_attention:get_component('stonehearth:firepit')
            if firepit_component and firepit_component:is_lit() then
               return true
            end
         end
      end

   end
   --To get here, there is either no lease,
   --or the lease is taken or it's not a fire
   --or it is but the fire isn't lit
   return false
end

--- Create a pathfinder to a lit fire
function AdmireFire:_start_looking_for_fire()
   assert (not self._pathfinder)

   --Find the nearest firepit seat
   local filter_fn = function(item)
      return self:_is_seat_by_lit_firepit(item)
   end

   --Once we've found a seat, grab its reservation
   local pf_solved_cb = function(path)
      self._pathfinder:stop()
      self._pathfinder = nil

      local firepit_seat = path:get_destination()

      --If we can lease the seat, great! If not restart
      local seat_lease = firepit_seat:get_component('stonehearth:lease_component')
      if seat_lease:try_to_acquire_lease(self._entity) then
         self._firepit_seat = firepit_seat
         self._path_to_fire = path
         self._ai:set_action_priority(self, 3)
      else
         --If the seat does have an owner, keep looking
         --TODO: get restart working. When I tried to pass it through from C++ and call
         --it from here, stuff just crashes
         -- self._pathfinder:restart()
         self:_start_looking_for_fire()
      end
   end

   -- go find the path to the fire
   local desc = string.format('finding firepit for %s', tostring(self._entity))
   self._pathfinder = radiant.pathfinder.create_path_finder(self._entity, desc)
                        :set_filter_fn(filter_fn)
                        :set_solved_cb(pf_solved_cb)
                        :find_closest_dst()
end

function AdmireFire:run(ai, entity)
   assert(self._path_to_fire)

   --Get the fire associated with the firepit
   local spot_component = self._firepit_seat:get_component('stonehearth:center_of_attention_spot')
   if not spot_component then
      ai:abort()
   end 
   self._firepit = spot_component:get_center_of_attention()

   -- If the firepit moves or is destroyed between now and before the
   -- sleeper wakes up, just go ahead and abort.
   self._firepit_moved_promise = radiant.entities.on_entity_moved(self._firepit, function()
      ai:abort()
   end);
   radiant.entities.on_destroy(self._firepit, function()
      --Can't call ai:abort, may not be on the correct thread.
      self._firepit = nil
   end);
   
   -- Go to the fire!
   ai:execute('stonehearth:follow_path', self._path_to_fire)

   --Am I carrying anything? If so, drop it
   local drop_location = self._path_to_fire:get_destination_point_of_interest()
   ai:execute('stonehearth:drop_carrying', drop_location)

   radiant.entities.turn_to_face(self._entity, spot_component:get_center_of_attention())

   self:_do_random_actions(ai)

end

--- Randomly pick an action to do in front of the fire
-- TODO: Add more sitting idle postures
-- TODO: Add a standing animation, so the loop can continue
function AdmireFire:_do_random_actions(ai)
   while true do
      if not self._firepit then
         ai:abort()
      end
      local random_action = math.random(100)
      if random_action < 30 then
         ai:execute('stonehearth:idle:breathe')
      elseif random_action > 80 and radiant.entities.get_posture(self._entity) ~= 'sitting' then
         radiant.entities.set_posture(self._entity, 'sitting')
         ai:execute('stonehearth:run_effect', 'sit_on_ground')
      elseif radiant.entities.get_posture(self._entity) ~= 'sitting' then
         --TODO: include these again if we have a standing animation
         ai:execute('stonehearth:idle:warm_hands')
      end
   end
end

function AdmireFire:_release_seat_reservation()
   if self._firepit_seat and self._firepit_seat:is_valid() then
      local seat_lease = self._firepit_seat:get_component('stonehearth:lease_component')
      seat_lease:release_lease(self._entity)
   end
   self._firepit_seat = nil
end

function AdmireFire:stop()
   self:_clear_variables()

   radiant.entities.unset_posture(self._entity, 'sitting')

   if self._should_look_for_fire then
      self:_start_looking_for_fire()
   end
end

function AdmireFire:_clear_variables()
   self._ai:set_action_priority(self, 0)

   if self._firepit_moved_promise then
      self._firepit_moved_promise:destroy()
      self._firepit_moved_promise = nil
   end

   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end

   --We now run this action for as long as we're standing beside the seat.
   --So when stopping, unconditionally release the lease.
   self:_release_seat_reservation()
end

return AdmireFire