local Entity = _radiant.om.Entity

local ConstructWorkshop = class()
ConstructWorkshop.name = 'complete workshop'
ConstructWorkshop.does = 'stonehearth:complete_workshop_construction'
ConstructWorkshop.args = {
   ghost_workshop = Entity,
   sound_effect = 'string'
}
ConstructWorkshop.version = 2
ConstructWorkshop.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(ConstructWorkshop)
            :execute('stonehearth:goto_entity', { entity = ai.ARGS.ghost_workshop })
            :execute('stonehearth:start_effect', { effect = ai.ARGS.sound_effect })
            :execute('stonehearth:run_effect', { effect = 'work' })
