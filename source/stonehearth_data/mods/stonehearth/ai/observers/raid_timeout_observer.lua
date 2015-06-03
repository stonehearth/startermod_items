-- Have the raiding party listen on a timeout, and after that timeout, have it depart

local RaidTimeoutObserver = class()

--Called once on creation
function RaidTimeoutObserver:initialize(entity)
   self._sv.entity = entity
end

--Always called. If restore, called after restore.
function RaidTimeoutObserver:activate()
   self._entity = self._sv.entity
   self._added_listener = radiant.events.listen(self._entity, 'stonehearth:game_master:citizen_config_complete', self, self._on_added)
end

--When this guy is added to the terrain, start the timer given the attributes he's got
function RaidTimeoutObserver:_on_added(e)
   if self._entity then 
      local attrib_component = self._entity:get_component('stonehearth:attributes')
      local timeout_base = attrib_component:get_attribute('raid_timeout_minutes')
      local timeout_variance = attrib_component:get_attribute('raid_timeout_variance_minutes')
      if timeout_base and timeout_variance then
         local timeout = tostring(timeout_base) .. 'm+' .. tostring(timeout_variance) .. 'm'
         self._sv.raid_timer = stonehearth.calendar:set_timer("RaidTimeoutObserver disperse timer", timeout, radiant.bind(self, '_on_timer_expire'))
         self.__saved_variables:mark_changed()
      end
   end   
   return radiant.events.UNLISTEN
end

--When the timer triggers, insert a high priority action that will have them disperse
--Note: the party seems to be using the stonehearth:urgent task group (make this clearer?)
function RaidTimeoutObserver:_on_timer_expire()
   local task = self._entity:add_component('stonehearth:ai')
                              :get_task_group('stonehearth:unit_control')
                                 :create_task('stonehearth:depart_visible_area')
                                    :set_priority(stonehearth.constants.priorities.party.DEPART_WORLD)
                                    :start()
   self:_stop_raid_timer()
end

function RaidTimeoutObserver:_stop_raid_timer()
   if self._sv.raid_timer then
      self._sv.raid_timer:destroy()
      self._sv.raid_timer = nil
      self.__saved_variables:mark_changed()
   end
end

function RaidTimeoutObserver:destroy()
   if self._added_listener then
      self._added_listener:destroy()
      self._added_listener = nil
   end
   self:_stop_raid_timer()
end

return RaidTimeoutObserver