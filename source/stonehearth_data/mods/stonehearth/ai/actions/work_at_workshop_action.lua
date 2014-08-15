local Entity = _radiant.om.Entity

local WorkAtWorkshop = class()
WorkAtWorkshop.name = 'work at workshop'
WorkAtWorkshop.does = 'stonehearth:work_at_workshop'
WorkAtWorkshop.args = {
   workshop = Entity,
   times = 'number',
   effect = 'string', 
   item_name = 'string'
}
WorkAtWorkshop.version = 2
WorkAtWorkshop.priority = 1

function WorkAtWorkshop:start(ai, entity, args)
   ai:set_status_text('crafting... ' .. args.item_name)
end

local ai = stonehearth.ai
return ai:create_compound_action(WorkAtWorkshop)
            :execute('stonehearth:drop_carrying_now')
            :execute('stonehearth:goto_entity', { entity = ai.ARGS.workshop })
            :execute('stonehearth:run_effect', { effect = ai.ARGS.effect, times = ai.ARGS.times, facing_entity = ai.ARGS.workshop})
