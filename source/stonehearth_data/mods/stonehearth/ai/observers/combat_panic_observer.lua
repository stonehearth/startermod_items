local log = radiant.log.create_logger('combat')

local CombatPanicObserver = class()

function CombatPanicObserver:initialize(entity, json)
   self._panic_threshold = radiant.util.get_config('panic_threshold', 0.50)
   self._entity = entity
   self._last_health = self:_get_health()

   radiant.events.listen(self._entity, 'stonehearth:attribute_changed:health', self, self._on_health_changed)
end

function CombatPanicObserver:destroy()
   radiant.events.unlisten(self._entity, 'stonehearth:attribute_changed:health', self, self._on_health_changed)
end

-- just using health right now, but this could depend on other attributes as well
-- panic every time health goes down and is below 50% max_health
function CombatPanicObserver:_on_health_changed()
   local current_health = self:_get_health()

   -- only declining health causes panic
   if current_health < self._last_health then
      -- max health might change (buffs, debuffs, etc), so get it every time
      local max_health = self:_get_max_health()
      if current_health / max_health <= self._panic_threshold then
         stonehearth.combat:set_panicking(self._entity, true)
      end
   end

   self._last_health = current_health
end

function CombatPanicObserver:_get_health()
   return radiant.entities.get_attribute(self._entity, 'health')
end

function CombatPanicObserver:_get_max_health()
   return radiant.entities.get_attribute(self._entity, 'max_health')
end

return CombatPanicObserver
