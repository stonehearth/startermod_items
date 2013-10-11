--[[
   Tell person to come and admire the fire
   This is an idle action, so it only happens when people have worked off their queue.
   TODO: Should this be inserted as people enter the world? How to handle actions that pop in and out?
   Right now it's on all humanoids
]]

local AdmireFire = class()

AdmireFire.name = 'admire fire'

--TODO: make a namespace for "not doing stuff"
AdmireFire.does = 'stonehearth:idle'
AdmireFire.priority = 0

function AdmireFire:__init(ai, entity)
   self._entity = entity
   self._ai = ai

   radiant.events.listen('radiant:events:calendar:sunrise', self)
   radiant.events.listen('radiant:events:calendar:sunset', self)
end

AdmireFire['radiant:events:calendar:sunset'] = function(self, calendar)

   --If we find a fire, we should mosey over
   local result_cb = function(fire, path)
      self._fire = fire
      self._path_to_fire = path
      --It's not REALLY high priority
      --TODO: bug--when we call follow path, its priority is just 1 instead of this 3
      --So the path following part is lowering priority than it should be
      self._ai:set_action_priority(self, 10)
   end

   if not self._pathfinder then
      self:_start_looking_for_fire(result_cb)
   end
end

AdmireFire['radiant:events:calendar:sunrise'] = function(self, calendar)
   self:stop()
end

function AdmireFire:_start_looking_for_fire(fire_found_cb)
   assert (not self._pathfinder)

   --Find the nearest fire
   local filter_fn = function(item)
      --radiant.log.info("looking for a fire!")
      local firepit_component = item:get_component('stonehearth:firepit')
      --TODO: get the faction of the firepit; only go to fires that belong to this faction
      return firepit_component ~= nil
   end

   local pf_solved_cb = function(path)
      local firepit = path:get_destination()
      -- TODO: reserve a place at the destination, so people don't stand overlapping
      fire_found_cb(firepit, path)
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
   assert(self._pathfinder)
   assert(self._path_to_fire)

   --Tell the pf to stop searching
   self._pathfinder:stop()
   self._pathfinder = nil

   -- Go to the fire!
   ai:execute('stonehearth:follow_path', self._path_to_fire)

   --Am I carrying anything? If so, drop it
   local drop_location = self._path_to_fire:get_destination_point_of_interest()
   ai:execute('stonehearth:drop_carrying', drop_location)


   radiant.entities.turn_to_face(self._entity, self._fire)
   local fire = self._path_to_fire:get_destination()
   self._last_fire = fire
end

function AdmireFire:stop()
   self._ai:set_action_priority(self, 0)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

return AdmireFire