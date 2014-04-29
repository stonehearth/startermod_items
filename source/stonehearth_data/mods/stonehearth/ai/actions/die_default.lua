local DieDefaultAction = class()

DieDefaultAction.name = 'default die action'
DieDefaultAction.does = 'stonehearth:die'
DieDefaultAction.args = {}
DieDefaultAction.version = 2
DieDefaultAction.priority = 1

local destroy_scheduled = {}

function DieDefaultAction:run(ai, entity, args)
   if destroy_scheduled[entity:get_id()] ~= nil then
      -- TODO: figure out what the AI should do after someone dies
      -- for now, don't infinite loop and don't allow lower priority AI to run
      ai:suspend()
      return
   end

   local name = radiant.entities.get_display_name(entity)
   local location = radiant.entities.get_world_grid_location(entity)

   ai:execute('stonehearth:drop_carrying_now')
   -- TODO: Improve the default death action
   --ai:execute('stonehearth:run_effect', { effect = 'die_peacefully'})

   radiant.terrain.remove_entity(entity)

   -- TODO: Add tombstone alternative text
   local tombstone = radiant.entities.create_entity('stonehearth:tombstone')
   radiant.entities.set_name(tombstone, 'RIP ' .. name)
   radiant.entities.set_description(tombstone, name .. ' will always be remembered')
   radiant.terrain.place_entity(tombstone, location)
   radiant.effects.run_effect(tombstone, '/stonehearth/data/effects/death')

   local name = radiant.entities.get_display_name(entity)
   stonehearth.events:add_entry(name .. ' has died.', 'warning')
end

function DieDefaultAction:stop(ai, entity, args)
   destroy_scheduled[entity:get_id()] = entity

   -- Shouldn't destroy the entity while the AI is running
   stonehearth.calendar:set_timer(1,
      function ()
         if entity:is_valid() then
            local id = entity:get_id()
            radiant.entities.destroy_entity(entity)
            destroy_scheduled[id] = nil
         end
      end
   )
end   

return DieDefaultAction
