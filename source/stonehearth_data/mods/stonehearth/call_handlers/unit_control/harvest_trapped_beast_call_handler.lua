local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local HarvestTrappedBeastCallHandler = class()


function HarvestTrappedBeastCallHandler:harvest_trapped_beast(session, response, entity, trap_entity)
   local scheduler = stonehearth.tasks:create_scheduler()
                                      :set_activity('stonehearth:unit_control', {})
                                      :join(entity)

   scheduler:create_task('stonehearth:unit_control:harvest_trapped_beast', { trap = trap_entity })
               :once()
               :add_entity_effect(trap_entity, '/stonehearth/data/effects/chop_overlay_effect')
               :start()
                         
   return true
end

return HarvestTrappedBeastCallHandler
