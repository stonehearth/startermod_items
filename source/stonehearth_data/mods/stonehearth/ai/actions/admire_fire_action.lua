--[[
   Tell person to come and admire the fire
   This is an idle action, so it only happens when people have worked off their queue,
   but it always happens, since at night, people naturally look for fire.
   TODO: rename to "find fireside seat or something"
   TODO: Separate idle from all other actions. Right now, it competes with the work and is immediately interrupted.
]]

local AdmireFire = class()

AdmireFire.name = 'stonehearth.actions.admire_fire'

--TODO: make a namespace for "not doing stuff"
AdmireFire.does = 'stonehearth.idle'
AdmireFire.priority = 0

function AdmireFire:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._firepit_seat = nil
   self._path_to_fire = nil

   radiant.events.listen('radiant.events.calendar.sunrise', self)
   radiant.events.listen('radiant.events.calendar.sunset', self)
end

--- At sunset, start looking for a seat near a fire
AdmireFire['radiant.events.calendar.sunset'] = function(self, calendar)
   if not self._pathfinder then
      self:_start_looking_for_fire()
   end
end

--- At dawn, stop looking and release all seats.
AdmireFire['radiant.events.calendar.sunrise'] = function(self, calendar)
   self:_release_seat_reservation()
   self:stop()
end

function AdmireFire:_start_looking_for_fire()
   assert (not self._pathfinder)

   --Find the nearest firepit seat
   local filter_fn = function(item)
      local item_uri = item:get_uri()
      local lease_component = item:get_component('stonehearth:lease_component')
      local leased = true
      if lease_component and (not lease_component:get_owner() or lease_component:get_owner():get_id() == self._entity:get_id()) then
         leased = false
      end
      return item_uri == 'stonehearth.fire_pit_seat' and not leased
   end

   --Once we've found a seat, grab its reservation
   local pf_solved_cb = function(path)
      local firepit_seat = path:get_destination()

      --Does the pf keep going after it's found one thing? To restart, hit restart
      local seat_lease = firepit_seat:get_component('stonehearth:center_of_attn_spot'):get_lease()
      if not seat_lease:get_owner() then
         --Great, grab the lease and go towards that seat
         seat_lease:lease_to(self._entity)
         self._firepit_seat = firepit_seat
         self._path_to_fire = path
         self._pathfinder:stop()
         self._pathfinder = nil

         self._ai:set_action_priority(self, 3)

      else
         --If the seat does have an owner, keep looking
         --TODO: get restart working. When I tried to pass it through from C++ and call
         --it from here, stuff just crashes
         -- self._pathfinder:restart()
         self._pathfinder:stop()
         self._pathfinder = nil
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
   ai:execute('stonehearth.follow_path', self._path_to_fire)

   --Am I carrying anything? If so, drop it
   local drop_location = self._path_to_fire:get_destination_point_of_interest()
   ai:execute('stonehearth.drop_carrying', drop_location)

   --Get the fire associated with the firepit
   local spot_component = self._firepit_seat:get_component('stonehearth:center_of_attn_spot')
   radiant.entities.turn_to_face(self._entity, spot_component:get_center_of_attn())
end

function AdmireFire:_release_seat_reservation()
   if self._firepit_seat then
      local seat_lease = self._firepit_seat:get_component('stonehearth:center_of_attn_spot'):get_lease()
      seat_lease:release_lease()
      self._firepit_seat = nil
   end
end

function AdmireFire:stop()
   self._ai:set_action_priority(self, 0)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end

   --If stop is called and the entity successfully is beside his seat, then stop
   if self._firepit_seat and
      not radiant.entities.is_adjacent_to(self._entity, radiant.entities.get_world_grid_location(self._firepit_seat)) then
      self:_release_seat_reservation()
   end

end

return AdmireFire