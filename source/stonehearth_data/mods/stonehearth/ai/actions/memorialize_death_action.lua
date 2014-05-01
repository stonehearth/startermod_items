local Point3 = _radiant.csg.Point3
local MemorializeDeathAction = class()

MemorializeDeathAction.name = 'memorialize death'
MemorializeDeathAction.does = 'stonehearth:memorialize_death'
MemorializeDeathAction.args = {
   name = 'string',
   location = Point3,
}
MemorializeDeathAction.version = 2
MemorializeDeathAction.priority = 1

-- consider separating all these 'effects' into a compound action
function MemorializeDeathAction:run(ai, entity, args)
   local title = string.format('RIP %s', args.name)
   local description = string.format('%s will always be remembered', args.name)
   local event_text = string.format('%s has died.', args.name)

   local tombstone = radiant.entities.create_entity('stonehearth:tombstone')
   radiant.entities.set_name(tombstone, title)
   radiant.entities.set_description(tombstone, description)
   radiant.terrain.place_entity(tombstone, args.location)

   radiant.effects.run_effect(tombstone, '/stonehearth/data/effects/death')

   stonehearth.events:add_entry(event_text, 'warning')
end

return MemorializeDeathAction
