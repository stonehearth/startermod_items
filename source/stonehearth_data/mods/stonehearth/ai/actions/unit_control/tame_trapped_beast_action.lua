local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local TameTrappedBeast = class()

TameTrappedBeast.name = 'tame trapped beast'
TameTrappedBeast.does = 'stonehearth:unit_control:tame_trapped_beast'
TameTrappedBeast.args = {
   trap = Entity      -- the trap containing the beast
}

TameTrappedBeast.version = 2
TameTrappedBeast.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

local ai = stonehearth.ai
return ai:create_compound_action(TameTrappedBeast)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.trap  })
         :execute('stonehearth:turn_to_face_entity', { entity = ai.ARGS.trap  })            
         :execute('stonehearth:run_effect', { effect = 'fiddle' })
         :execute('stonehearth:unit_control:tame_trapped_beast_adjacent', { trap = ai.ARGS.trap })
