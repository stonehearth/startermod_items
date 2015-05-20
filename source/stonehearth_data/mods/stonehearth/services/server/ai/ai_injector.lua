local AiInjector = class()

function AiInjector:__init(entity, ai)
   self._log = radiant.log.create_logger('ai.injector')
                              :set_entity(entity)
   self._entity = entity
   self._injected = {}
   self._injected.actions = {}
   self._injected.observers = {}

   self:inject_ai(ai)
end

function AiInjector:inject_ai(ai)
   self._log:info('injecting ai into %s', self._entity)
   
   if ai.ai_packs then
      for _, uri in pairs(ai.ai_packs) do
         self:_inject_ai_pack(uri)
      end
   end

   if ai.actions then
      local aic = self._entity:add_component('stonehearth:ai')
      for _, uri in ipairs(ai.actions) do
         if not self._injected.actions[uri] then
            aic:add_action(uri)
            self._injected.actions[uri] = uri
         end
      end
   end

   if ai.observers then
      local obs = self._entity:add_component('stonehearth:observers')
      for _, uri in ipairs(ai.observers) do
         if not self._injected.observers[uri] then
            obs:add_observer(uri)
            self._injected.observers[uri] = uri
         end
      end
   end
end

function AiInjector:_inject_ai_pack(uri)
   local ai = radiant.resources.load_json(uri, true)
   self:inject_ai(ai)
end

function AiInjector:destroy()
   if not self._entity:is_valid() then
      self._log:info('entity destroyed before revoking injected ai', self._entity)
      return
   end

   self._log:info('revoking injected ai')

   if self._injected.actions then
      local ai = self._entity:add_component('stonehearth:ai')
      for uri in pairs(self._injected.actions) do
         ai:remove_action(uri)
      end
   end

   if self._injected.observers then
      local obs = self._entity:add_component('stonehearth:observers')
      for uri in pairs(self._injected.observers) do
         obs:remove_observer(uri)
      end
   end
end

return AiInjector