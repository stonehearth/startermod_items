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
   missing_ingredients: data about the ingredients we still need to collect
                        (some may already be on the workbench)
]]
function GatherAndCraftAction:run(ai, entity, recipe, ingredients)
   local crafter_component = self._entity:get_component('stonehearth_crafter:crafter')
   local workshop = crafter_component:get_workshop()
   local workshop_entity = workshop:get_entity()
   local workshop_ec = workshop_entity:add_component('entity_container'):get_children()
   for _, ing_data in ipairs(ingredients) do

      --If the user has paused progress, or cancelled the orderdon't continue.
      --Stopping will cause the  list to be reevaluated from scratch
      if workshop:is_paused() or (workshop:get_curr_order() == nil) then
         return
      end

      local item = ing_data.item
      -- is the item already on the bench?  if so, there's nothing to do.
      if not workshop_ec:get(item:get_id()) then
         -- grab it!
         ai:execute('stonehearth.activities.pickup_item', ing_data.item)

         -- bring it back!
         ai:execute('stonehearth.activities.goto_entity', workshop_entity)

         -- drop it!!
         ai:execute('stonehearth.activities.run_effect', 'carry_putdown_on_table')
         radiant.entities.add_child(workshop_entity, item, RadiantIPoint3(0, 1, 0))
         entity:get_component('carry_block'):set_carrying(nil)
      end
   end
   -- Once everything's gathered, then craft
   ai:execute('stonehearth_crafter.activities.craft')
end

return GatherAndCraftAction
