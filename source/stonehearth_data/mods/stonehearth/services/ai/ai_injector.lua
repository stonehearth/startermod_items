local AiInjector = class()
local log = radiant.log.create_logger('ai.injector')

-- injecting_entity is optional
function AiInjector:__init(entity, ai, injecting_entity)
   self._entity = entity
   self._injecting_entity = injecting_entity
   self._injected = {}
   self._injected.actions = {}
   self._injected.observers = {}

   self:inject_ai(ai)
end

function AiInjector:inject_ai(ai)
   log:info('injecting ai into %s', tostring(self._entity))
   local ai_component = self._entity:add_component('stonehearth:ai')

   if ai.actions then
      for _, uri in ipairs(ai.actions) do
         ai_component:add_action(uri, self._injecting_entity)
         table.insert(self._injected.actions, uri)
      end
   end

   if ai.observers then
      for _, uri in ipairs(ai.observers) do
         ai_component:add_observer(uri, self._injecting_entity)
         table.insert(self._injected.observers, uri)
      end
   end
   -- instantiate the actions and observers, and bookkeeping
end

function AiInjector:destroy()
   log:info('revoking injected ai from %s', tostring(self._entity))
   local ai_component = self._entity:add_component('stonehearth:ai')

   if self._injected.actions then
      for _, uri  in ipairs(self._injected.actions) do
         ai_component:remove_action(uri)
      end
   end

   if self._injected.observers then
      for _, uri  in ipairs(self._injected.observers) do
         ai_component:remove_observer(uri)
      end
   end
end

return AiInjector