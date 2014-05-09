local Entity = _radiant.om.Entity

local WorkAtWorkshop = class()
WorkAtWorkshop.name = 'work at workshop'
WorkAtWorkshop.does = 'stonehearth:work_at_workshop'
WorkAtWorkshop.args = {
   workshop = Entity,
   times = 'number',
   effect = 'string'
}
WorkAtWorkshop.version = 2
WorkAtWorkshop.priority = 1

function WorkAtWorkshop:start(ai, entity, args)
   ai:set_status_text('crafting...(insert item being crafter here)')
end

local ai = stonehearth.ai
return ai:create_compound_action(WorkAtWorkshop)
            :execute('stonehearth:goto_entity', { entity = ai.ARGS.workshop })
            :execute('stonehearth:run_effect', { effect = ai.ARGS.effect, times = ai.ARGS.times, facing_entity = ai.ARGS.workshop})
