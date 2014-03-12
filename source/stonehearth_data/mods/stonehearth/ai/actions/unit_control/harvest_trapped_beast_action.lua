local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local HarvestTrappedBeast = class()

HarvestTrappedBeast.name = 'harvest trapped beast'
HarvestTrappedBeast.does = 'stonehearth:unit_control:harvest_trapped_beast'
HarvestTrappedBeast.args = {
   trap = Entity      -- the trap containing the beast
}

HarvestTrappedBeast.version = 2
HarvestTrappedBeast.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

local ai = stonehearth.ai
return ai:create_compound_action(HarvestTrappedBeast)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.trap  })
         :execute('stonehearth:turn_to_face_entity', { entity = ai.ARGS.trap  })            
         :execute('stonehearth:run_effect', { effect = 'fiddle' })
         :execute('stonehearth:unit_control:harvest_trapped_beast_adjacent', { trap = ai.ARGS.trap })
