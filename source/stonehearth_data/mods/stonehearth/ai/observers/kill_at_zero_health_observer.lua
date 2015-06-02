--[[
   Observes the HP of the entity and shows popup toasts based on their current HP
]]

local KillAtZeroHealthObserver = class()

--Called once on creation
function KillAtZeroHealthObserver:initialize(entity)
   self._sv.entity = entity
end

--Always called. If restore, called after restore.
function KillAtZeroHealthObserver:activate()
   self._entity = self._sv.entity
   self._attributes_component = self._entity:add_component('stonehearth:attributes')
   self._listener = radiant.events.listen(self._entity, 'stonehearth:attribute_changed:health', self, self._on_health_changed)
end

function KillAtZeroHealthObserver:destroy()
   if self._listener then
      self._listener:destroy()
      self._listener = nil
   end
end

function KillAtZeroHealthObserver:_on_health_changed(e)
   local health = self._attributes_component:get_attribute('health')
   if health <= 0 then
      self._listener:destroy()
      radiant.entities.kill_entity(self._entity)
   end
end

return KillAtZeroHealthObserver
