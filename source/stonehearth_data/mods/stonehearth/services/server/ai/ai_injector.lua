local AiInjector = class()

function AiInjector:initialize(entity, ai)
   self._sv.entity = entity
   self._sv._injected = {
      actions = {},
      observers = {},
   }

   self._log = radiant.log.create_logger('ai.injector')
                              :set_entity(self._sv.entity)
                              
   self:_flatten_ai(ai)
   self:_inject_ai()
end

function AiInjector:restore()
   self._log = radiant.log.create_logger('ai.injector')
                              :set_entity(self._sv.entity)

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
         self:_restore_ai()
      end)
end

function AiInjector:activate(entity, ai)
end

function AiInjector:_flatten_ai(ai)
   if ai.actions then
      for _, uri in ipairs(ai.actions) do
         self._sv._injected.actions[uri] = true
      end
   end

   if ai.observers then
      for _, uri in ipairs(ai.observers) do
         self._sv._injected.observers[uri] = true
      end
   end

   if ai.ai_packs then
      for _, uri in pairs(ai.ai_packs) do
         self:_flatten_ai_pack(uri)
      end
   end
end

function AiInjector:_flatten_ai_pack(uri)
   local ai = radiant.resources.load_json(uri, true)
   self:_flatten_ai(ai)
end

function AiInjector:_inject_ai()
   self:_inject_actions()
   self:_inject_observers()
end

function AiInjector:_restore_ai()
   -- Don't restore observers on load because they are controllers themselves and are self restored
   -- "Your lack of symmetry disturbs me." -- Vader
   self:_inject_actions()
end

function AiInjector:_inject_actions()
   local aic = self._sv.entity:add_component('stonehearth:ai')
   for uri in pairs(self._sv._injected.actions) do
      aic:add_action(uri)
   end
end

function AiInjector:_inject_observers()
   local obs = self._sv.entity:add_component('stonehearth:observers')
   for uri in pairs(self._sv._injected.observers) do
      obs:add_observer(uri)
   end
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
