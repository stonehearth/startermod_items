local EffectManager = require 'modules.effects.effect_manager'

local effects = {}
local singleton = {}

function effects.__init()
   singleton._all_effects = {}
end

function effects._on_event_loop(_, e)
   local now = e.now
   for id, mgr in pairs(singleton._all_effects) do
      mgr:on_event_loop(e)
   end
end

function effects.run_effect(entity, effect_name, ...)
   radiant.check.is_entity(entity)
   radiant.check.is_string(effect_name)
   
   local id = entity:get_id()
   local effect_mgr = singleton._all_effects[id]
   
   if not effect_mgr then
      effect_mgr = EffectManager(entity)
      singleton._all_effects[id] = effect_mgr
   end
   return effect_mgr:start_effect(effect_name, ...)
end

radiant.events.listen(radiant, 'stonehearth:gameloop', effects, effects._on_event_loop)

effects.__init()
return effects
