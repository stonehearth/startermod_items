--[[
   Listens for various events and triggers free-time-priority actions on those events. 
   For example, people only look for fires to admire if there are fires. 
]]

local events = stonehearth.events

local FreeTimeObserver = class()

function FreeTimeObserver:__init(entity)
end

function FreeTimeObserver:initialize(entity, json)
   self._entity = entity

   self._fire_task = nil

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv._num_fires = 0
      self._sv.should_find_fires = false
   end

   --Listen on fire added/removed events
   radiant.events.listen(events, 'stonehearth:fire:lit', self, self._on_firepit_activity)

   if self._sv.should_find_fires then
      self:_start_admiring_fire_task()
   end
end

--If there is a lit fire somewhere, consider hanging out there
--If there are no lit fires anywhere, don't look for fires. 
function FreeTimeObserver:_on_firepit_activity(e)
   --TODO: I suppose it might make sense for him to only do this IF there were fires relatively nearby
   if e.lit and e.player_id == radiant.entities.get_player_id(self._entity) then
      self._sv._num_fires = self._sv._num_fires + 1
      self:_start_admiring_fire_task()
   elseif not e.lit and e.player_id == radiant.entities.get_player_id(self._entity) then
      self._sv._num_fires = self._sv._num_fires - 1
      if self._sv._num_fires <= 0 then
         self:_finish_admiring()
      end
   end

end

function FreeTimeObserver:_start_admiring_fire_task()
   if not self._fire_task then
      -- get the task group "free_time" and create a task to admire the fire at the right priority
      self._fire_task = self._entity:get_component('stonehearth:ai')
                           :get_task_group('stonehearth:free_time')
                              :create_task('stonehearth:admire_fire', {})
                                 :set_priority(stonehearth.constants.priorities.free_time.ADMIRE_FIRE)
                                 :start()
      self._sv.should_find_fires = true
      self.__saved_variables:mark_changed()
   end
end

function FreeTimeObserver:_finish_admiring()
   self._sv.should_find_fires = false
   self.__saved_variables:mark_changed()

   self._fire_task:destroy()
   self._fire_task = nil
end

return FreeTimeObserver