local Entity = _radiant.om.Entity

local WorkAtWorkshop = class()
WorkAtWorkshop.name = 'work at workshop'
WorkAtWorkshop.does = 'stonehearth:work_at_workshop'
WorkAtWorkshop.args = {
   workshop = Entity,
   times = 'number'
}
WorkAtWorkshop.version = 2
WorkAtWorkshop.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(WorkAtWorkshop)
            :execute('stonehearth:goto_entity', { entity = ai.ARGS.workshop })
            :execute('stonehearth:run_effect', { effect = ai.ENTITY:get_component('stonehearth:crafter'):get_work_effect(), times = ai.ARGS.times})
