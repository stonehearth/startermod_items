-- Entity gets an event when the cage is destroyed 
-- BY default, entity is sleeping, when the cage is destroyed, entity flees world

local CagedEntityComponent = class()

function CagedEntityComponent:initialize(entity, json)
   self._entity = entity
   if not self._sv.initialized then
      self._sv.initialized = true
      self:_sleep_in_cage()
   else
      if self._sv.cage then
         self:_listen_on_cage_kill()
      end
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
         self:_sleep_in_cage()
      end)
   end
end

function CagedEntityComponent:destroy()
   if self._cage_listener then
      self._cage_listener:destroy()
      self._cage_listener = nil
   end
end

function CagedEntityComponent:set_cage(cage_entity)
   self._sv.cage = cage_entity

   --Listen on cage kill
   self:_listen_on_cage_kill()
end

function CagedEntityComponent:_listen_on_cage_kill()
   assert(self._sv.cage and self._sv.cage:is_valid())
   self._cage_listener = radiant.events.listen(self._sv.cage, 'stonehearth:kill_event', self, self._on_cage_killed)
   self._cage_destroy_listener = radiant.events.listen(self._sv.cage, 'radiant:entity:pre_destroy', self, self._on_cage_destroyed)
end

--If the cage is killed, have the people inside disperse immediately
--TODO: consider what we want if they just flee, and then don't leave the world. 
function CagedEntityComponent:_on_cage_killed(e)
   if self._sleep_task then
      self._sleep_task:destroy()
      self._sleep_task = nil
   end

   self._depart_task = self._entity:add_component('stonehearth:ai')
            :get_task_group('stonehearth:compelled_behavior')
            :create_task('stonehearth:depart_visible_area')
            :set_priority(stonehearth.constants.priorities.compelled_behavior.DEPART_WORLD)
            :start()
end

--When the cage is destroyed, wake up
function CagedEntityComponent:_on_cage_destroyed()
   if self._sleep_task then
      self._sleep_task:destroy()
      self._sleep_task = nil
   end
end

--Things in cages always just sleep for now. 
--The sleep task is always lower priority than the depart world task
function CagedEntityComponent:_sleep_in_cage()
   self._sleep_task = self._entity:add_component('stonehearth:ai')
            :get_task_group('stonehearth:compelled_behavior')
            :create_task('stonehearth:sleep_exhausted')
            :set_priority(stonehearth.constants.priorities.compelled_behavior.IMPRISONED)
            :start()
end

return CagedEntityComponent