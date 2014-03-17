local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('pets')

local TrappedBeastCallHandler = class()

function TrappedBeastCallHandler:harvest_trapped_beast(session, response, entity, trap_entity)
   if not radiant.util.is_a(entity, Entity) or not radiant.util.is_a(trap_entity, Entity) then
      -- entity no longer exists
      return false
   end
   local town = stonehearth.town:get_town(session.player_id)
   town:command_unit_scheduled(entity, 'stonehearth:unit_control:harvest_trapped_beast', { trap = trap_entity })
            :add_entity_effect(trap_entity, '/stonehearth/data/effects/trapped_beast_overlay_effect/harvest_trapped_beast_overlay_effect.json')
            :once()
            :start()

   return true
end

function TrappedBeastCallHandler:tame_trapped_beast(session, response, entity, trap_entity)
   if not radiant.util.is_a(entity, Entity) or not radiant.util.is_a(trap_entity, Entity) then
      -- entity no longer exists
      return false
   end

   local town = stonehearth.town:get_town(session.player_id)
   town:command_unit_scheduled(entity, 'stonehearth:unit_control:tame_trapped_beast', { trap = trap_entity })
            :add_entity_effect(trap_entity, '/stonehearth/data/effects/trapped_beast_overlay_effect/tame_trapped_beast_overlay_effect.json')
            :once()
            :start()

   return true
end

return TrappedBeastCallHandler
