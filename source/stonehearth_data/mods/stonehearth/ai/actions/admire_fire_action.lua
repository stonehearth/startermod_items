--[[
   Tell person to come and admire the fire
   This is an idle action, so it only happens when people have worked off their queue,
   but it always happens, since at night, people naturally look for fire.
   TODO: rename to "find fireside seat or something"
   TODO: Separate idle from all other actions. Right now, it competes with the work and is immediately interrupted.
]]

local AdmireFire = class()

AdmireFire.name = 'admire fire'

--TODO: make a namespace for "not doing stuff"
AdmireFire.does = 'stonehearth:idle'
AdmireFire.priority = 0

function AdmireFire:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._firepit_seat = nil
   self._path_to_fire = nil
   self._should_look_for_fire = false

   radiant.events.listen('radiant:events:calendar:sunrise', self)
   radiant.events.listen('radiant:events:calendar:sunset', self)
end

AdmireFire['radiant:events:calendar:sunset'] = function(self, calendar)
   self._should_look_for_fire = true
   if not self._pathfinder then
      self:_start_looking_for_fire()
   end
end

--- At dawn, stop looking and release all seats.
AdmireFire['radiant:events:calendar:sunrise'] = function(self, calendar)
   self._should_look_for_fire = false
   self:_release_seat_reservation()
   self._ai:set_action_priority(self, 0)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

function AdmireFire:_start_looking_for_fire()
   assert (not self._pathfinder)

   --Find the nearest firepit seat
   local filter_fn = function(item)
      local item_uri = item:get_uri()
      local lease_component = item:get_component('stonehearth:lease_component')
      local leased = true
      if lease_component and lease_component:can_acquire_lease(self._entity) then
         leased = false
      end
      return item_uri == 'stonehearth:fire_pit_seat' and not leased
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
   self._pathfinder = radiant.pathfinder.create_path_finder(desc)
                        :set_source(self._entity)
                        :set_filter_fn(filter_fn)
                        :set_solved_cb(pf_solved_cb)
                        :find_closest_dst()
end

function AdmireFire:run(ai, entity)
   assert(self._path_to_fire)

   -- Go to the fire!
   ai:execute('stonehearth:follow_path', self._path_to_fire)

   --Am I carrying anything? If so, drop it
   local drop_location = self._path_to_fire:get_destination_point_of_interest()
   ai:execute('stonehearth:drop_carrying', drop_location)

   --Get the fire associated with the firepit
   local spot_component = self._firepit_seat:get_component('stonehearth:center_of_attention_spot')
   radiant.entities.turn_to_face(self._entity, spot_component:get_center_of_attention())

   while true do
      --TODO: add the fire-specific stuff
      ai:execute('stonehearth:idle')
      local effects = {
         'warm_hands_by_fire',
         'toast_marshmellow'
      }
      --ai:execute('stonehearth:run_effect', effects[math.random(#effects)])
   end
end

function AdmireFire:_release_seat_reservation()
   if self._firepit_seat then
      local seat_lease = self._firepit_seat:get_component('stonehearth:lease_component')
      seat_lease:release_lease(self._entity)
      self._firepit_seat = nil
   end
end

function AdmireFire:stop()
   self._ai:set_action_priority(self, 0)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._should_look_for_fire then
      self:_start_looking_for_fire()
   end

   --We now run this function for as long as we're standing beside the seat.
   --So when stopping, unconditionally release the lease.
   --If stop is called and the entity successfully is beside his seat, then stop
   if self._firepit_seat then
      self._firepit_seat:get_component('stonehearth:lease_component'):release_lease(self._entity)
   end

end

return AdmireFire