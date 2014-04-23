--[[
   If we fall under 0 HP run this action 
]]

local DieDefaultAction = class()

DieDefaultAction.name = 'default die action'
DieDefaultAction.does = 'stonehearth:die'
DieDefaultAction.args = {}
DieDefaultAction.version = 2
DieDefaultAction.priority = 1

function DieDefaultAction:run(ai, entity)
   local name = radiant.entities.get_display_name(entity)
   local location = radiant.entities.get_world_grid_location(entity)

   ai:execute('stonehearth:drop_carrying_now')
   -- TODO: Improve the default death action
   ai:execute('stonehearth:run_effect', { effect = 'die_peacefully'})

   radiant.terrain.remove_entity(entity)
   
   -- TODO: Add tombstone alternative text
   local tombstone_entity = radiant.entities.create_entity('stonehearth:tombstone')
   radiant.entities.set_name(tombstone_entity, 'RIP ' .. name)
   radiant.entities.set_description(tombstone_entity, name .. ' will always be remembered')
   radiant.terrain.place_entity(tombstone_entity, location)
   radiant.effects.run_effect(tombstone_entity, '/stonehearth/data/effects/death')

   -- Write death to the UI
   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' has died.', 'warning')

end

function DieDefaultAction:stop(ai, entity)
   radiant.entities.destroy_entity(entity)
end   


return DieDefaultAction