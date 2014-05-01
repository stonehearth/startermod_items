local DieAction = class()

DieAction.name = 'die action'
DieAction.does = 'stonehearth:die'
DieAction.args = {}
DieAction.version = 2
DieAction.priority = 1

local destroy_scheduled = {}

function DieAction:run(ai, entity, args)
   if entity == nil or
      not entity:is_valid() or 
      destroy_scheduled[entity:get_id()] ~= nil then
      -- TODO: figure out what the AI should do after someone dies
      -- for now, don't infinite loop and don't allow lower priority AI to run
      ai:suspend()
      return
   end

   -- these actions shouldn't be performed as part of a compound action
   -- if they fail, we don't want to abort the death / destroy_entity
   ai:execute('stonehearth:drop_carrying_now')
   -- TODO: Improve the default death action
   --ai:execute('stonehearth:run_effect', { effect = 'die_peacefully'})

   local name = radiant.entities.get_display_name(entity)
   local location = radiant.entities.get_world_grid_location(entity)

   radiant.terrain.remove_entity(entity)

   ai:execute('stonehearth:memorialize_death', { name = name, location = location })
end

function DieAction:stop(ai, entity, args)
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

return DieAction
