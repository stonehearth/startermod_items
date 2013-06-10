--[[
   Gather all the materials necessary to craft the recipe.
   If the gathering is interrupted, all collected things
   stay on the workbench, but on resume, the worker evaluates 
   a new recipe (and does not attempt to finish the recipe
   he was halfway through gathering).

   Pros: If we're interrupted halfway through gathering, and
   the remaining stuff needed is no longer in the world when 
   we resume, we can just start a new recipe and reuse some 
   of the gathered things. 
   Cons: We may have gathered a bunch of things that are no longer
   useful, and that now only he can use.  
]]

local GatherAndCraftAction = class()

GatherAndCraftAction.name = 'stonehearth.actions.gather_and_craft'
GatherAndCraftAction.does = 'stonehearth_crafter.activities.gather_and_craft'
GatherAndCraftAction.priority = 5

function GatherAndCraftAction:__init(ai, entity)
   self._entity = entity
end

--[[
   Called when the crafter wants to start a new recipe
   and must gather all the items.
   recipe:  recipe for the thing we're about to craft
   all_ing: data about the ingredients we need to collect

   TODO: finalize the inventory/all_ing api
]]
function GatherAndCraftAction:run(ai, entity, recipe, all_ing)
   local crafter_component = self._entity:get_component('mod://stonehearth_crafter/components/crafter.lua')
   local workshop = crafter_component:get_workshop()

   for _, ing_data in ipairs(all_ing) do
      local object_location = radiant.entities.get_world_grid_location(ing_data.item)
      ai:execute('stonehearth.activities.goto_location', object_location)
      --TODO: make pickup action (something like --ai:execute('radiant.actions.pickup_item', ing_data.item)  )
      --TODO: stand beside workshop, something like radiant.entities.get_world_grid_location(workshop:get_entity()):standable_loc()
      object_location = RadiantIPoint3(-12, 1, -10)
      ai:execute('stonehearth.activities.goto_location', object_location)
      --TODO: make a drop on object action, something like(ai.execute('stonehearth.activities.run_effect', 'drop_on_workbench') )
      --TODO: give the bench entity_container
      workshop:add_item_to_bench(ing_data.item)  
      
   end
   -- Once everything's gathered, then craft
   ai:execute('stonehearth_crafter.activities.craft')
end

return GatherAndCraftAction
