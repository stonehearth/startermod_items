local AiInjector = class()

function AiInjector:initialize(entity, ai)
   self._log = radiant.log.create_logger('ai.injector')
                              :set_entity(self._sv.entity)
                              
   self._sv.entity = entity
   self._sv._injected = {
      actions = {},
      observers = {},
   }
   self:inject_ai(ai)
end

function AiInjector:restore()
   self._log = radiant.log.create_logger('ai.injector')
                              :set_entity(self._sv.entity)
end

function AiInjector:activate(entity, ai)
                              
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
         self:_reinject_ai()
      end)
end

function AiInjector:inject_ai(ai)
   self._log:info('injecting ai into %s', self._sv.entity)
   
   if ai.ai_packs then
      for _, uri in pairs(ai.ai_packs) do
         self:_inject_ai_pack(uri)
      end
   end

   if ai.actions then
      local aic = self._sv.entity:add_component('stonehearth:ai')
      for _, uri in ipairs(ai.actions) do
         if not self._sv._injected.actions[uri] then
            aic:add_action(uri)
            self._sv._injected.actions[uri] = true
         end
      end
   end

   if ai.observers then
      local obs = self._sv.entity:add_component('stonehearth:observers')
      for _, uri in ipairs(ai.observers) do
         if not self._sv._injected.observers[uri] then
            obs:add_observer(uri)
            self._sv._injected.observers[uri] = true
         end
      end
   end
end

function AiInjector:_reinject_ai()
   local aic = self._sv.entity:add_component('stonehearth:ai')
   for uri in pairs(self._sv._injected.actions) do
      aic:add_action(uri)
   end

   local obs = self._sv.entity:add_component('stonehearth:observers')
   for uri in pairs(self._sv._injected.observers) do
      obs:add_observer(uri)
   end
end

function AiInjector:_inject_ai_pack(uri)
   local ai = radiant.resources.load_json(uri, true)
   self:inject_ai(ai)
end

function AiInjector:destroy()
   if not self._sv.entity:is_valid() then
      self._log:info('entity destroyed before revoking injected ai', self._sv.entity)
      return
   end

   self._log:info('revoking injected ai')

   local ai = self._sv.entity:add_component('stonehearth:ai')
   for uri in pairs(self._sv._injected.actions) do
      ai:remove_action(uri)
   end

   local obs = self._sv.entity:add_component('stonehearth:observers')
   for uri in pairs(self._sv._injected.observers) do
      obs:remove_observer(uri)
   end
end

return AiInjector
