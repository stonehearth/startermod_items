local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local HarvestTrappedBeastCallHandler = class()


function HarvestTrappedBeastCallHandler:harvest_trapped_beast(session, response, entity, trap_entity)
   local town = stonehearth.town:get_town(session.faction)
   town:command_unit_scheduled(entity, 'stonehearth:unit_control:harvest_trapped_beast', { trap = trap_entity })
            :add_entity_effect(trap_entity, '/stonehearth/data/effects/harvest_trapped_beast_overlay_effect')
            :once()
            :start()
     
   return true
end

return HarvestTrappedBeastCallHandler
