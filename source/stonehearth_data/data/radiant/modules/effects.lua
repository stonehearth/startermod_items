local EffectManager = require 'radiant.modules.effects.effect_manager'

local effects = {}
local singleton = {}

function effects.__init()
   singleton._all_effects = {}
end

--[[
function effects._init_entity(entity, resource)
   radiant.check.is_entity(entity)
   
   local id = entity:get_id()
   if not singleton._all_effects[id] then
      singleton._all_effects[id] = EffectManager(entity, resource)
      radiant.entities.on_destroy(entity, function()
         singleton._all_effects[id]:destroy(entity)
         singleton._all_effects[id] = nil
      end)
   end
   return singleton._all_effects[id]
end
]]

function effects._on_event_loop(now)
   for id, mgr in pairs(singleton._all_effects) do
      mgr:on_event_loop()
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
      radiant.entities.on_destroy(entity, function()
         singleton._all_effects[id]:destroy(entity)
         singleton._all_effects[id] = nil
      end)
   end
   return effect_mgr:start_effect(effect_name, ...)
end

radiant.events.register_event('radiant.animation.on_trigger')
radiant.events.register_event('radiant.effect.frame_data.on_segment')
radiant.events.listen('radiant.events.gameloop', effects._on_event_loop)

effects.__init()
return effects
