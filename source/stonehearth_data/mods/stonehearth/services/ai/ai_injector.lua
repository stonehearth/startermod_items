local AiInjector = class()

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
   radiant.log.info('injected ai!')
   local ai_service = radiant.mods.load('stonehearth').ai

   if ai.actions then
      for _, uri in ipairs(ai.actions) do
         local action = ai_service:add_action(self._entity, uri, self._injecting_entity)
         table.insert(self._injected.actions, uri)
      end
   end

   if ai.observers then
      for _, uri in ipairs(ai.observers) do
         local observer = ai_service:add_observer(self._entity, uri, self._injecting_entity)
         table.insert(self._injected.observers, observer)
      end
   end
   -- instantiate the actions and observers, and bookkeeping
end

function AiInjector:destroy()
   radiant.log.info('revoking injected ais!')
   local ai_service = radiant.mods.load('stonehearth').ai

   if self._injected.actions then
      for _, uri  in ipairs(self._injected.actions) do
         ai_service:remove_action(self._entity, uri)
      end
   end

   if self._injected.observers then
      for _, observer  in ipairs(self._injected.observers) do
         ai_service:remove_observer(self._entity, observer)
      end
   end
end

return AiInjector