local AiInjector = class()

function AiInjector:__init(entity, ai, injecting_entity)
   self._log = radiant.log.create_logger('ai.injector')
                              :set_entity(entity)
   self._entity = entity
   self._injecting_entity = injecting_entity
   self._injected = {}
   self._injected.actions = {}
   self._injected.observers = {}

   self:inject_ai(ai)
end

function AiInjector:inject_ai(ai)
   self._log:info('injecting ai into %s', self._entity)
   

   if ai.actions then
      local aic = self._entity:add_component('stonehearth:ai')
      for _, uri in ipairs(ai.actions) do
         aic:add_action(uri, self._injecting_entity)
         table.insert(self._injected.actions, uri)
      end
   end

   if ai.observers then
      local obs = self._entity:add_component('stonehearth:observers')
      for _, uri in ipairs(ai.observers) do
         obs:add_observer(uri, self._injecting_entity)
         table.insert(self._injected.observers, uri)
      end
   end
   -- instantiate the actions and observers, and bookkeeping
end

function AiInjector:destroy()
   if not self._entity:is_valid() then
      self._log:info('entity destroyed before revoking injected ai', self._entity)
      return
   end

   self._log:info('revoking injected ai')

   if self._injected.actions then
      local ai = self._entity:add_component('stonehearth:ai')
      for _, uri  in ipairs(self._injected.actions) do
         ai:remove_action(uri)
      end
   end

   if self._injected.observers then
      local obs = self._entity:add_component('stonehearth:observers')
      for _, uri  in ipairs(self._injected.observers) do
         obs:remove_observer(uri)
      end
   end
end

return AiInjector