local rng = _radiant.csg.get_default_rng()

local ChooseLocationOutsideTown = class()

function ChooseLocationOutsideTown:initialize(player_id, min_range, max_range, callback)
   checks('self', 'string', 'number', 'number', 'binding')

   self._sv.player_id = player_id
   self._sv.min_range = min_range
   self._sv.max_range = max_range
   self._sv.callback  = callback
   self._sv.found_location = false
end

function ChooseLocationOutsideTown:activate()
   self._log = radiant.log.create_logger('game_master')
                              :set_prefix('choose loc outside city')

   self:_destroy_find_location_timer()


   -- we use a goblin for pathfinding.  sigh.  ideally we should be able to pass in a mob type
   -- (e.g. HUMANOID) so we avoid creating this guy
   -- Only create this guy if we really need him!
   if not self._sv.find_location_entity and not self._sv.found_location then
      self._sv.find_location_entity = radiant.entities.create_entity('stonehearth:monsters:goblins:goblin')
   end
   if not self._sv.found_location then
      self:_try_finding_location()
   end
end

function ChooseLocationOutsideTown:_try_finding_location()
      -- choose a distance and start looking
   local d = rng:get_int(self._sv.min_range, self._sv.max_range)
   local player_banner = stonehearth.town:get_town(self._sv.player_id)
                                             :get_banner()
   local success = function(location)
      self._log:info('found location %s', location)
      self:_finalize_location(location)
   end

   local fail = function()
      self._log:info('failed to find location.  trying again')
      self:_try_later()
   end

   stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity_astar(
      self._sv.find_location_entity, player_banner, d, 1, d * 100, success, fail) 
end


function ChooseLocationOutsideTown:_finalize_location(location)
   self:_destroy_find_location_timer()
   radiant.entities.destroy_entity(self._sv.find_location_entity)

   self._sv.find_location_entity = nil
   self._sv.found_location = true
   self.__saved_variables:mark_changed()
   if self._sv.callback then
      radiant.invoke(self._sv.callback, location)
      self._sv.callback = nil
   end
end

function ChooseLocationOutsideTown:_destroy_find_location_timer()
   if self._find_location_timer then
      self._find_location_timer:destroy()
      self._find_location_timer = nil
   end
end

function ChooseLocationOutsideTown:_try_later()
   self:_destroy_find_location_timer()
   self._find_location_timer = radiant.set_realtime_timer(5000, function()
         self:_try_finding_location()
      end)
end

return ChooseLocationOutsideTown
