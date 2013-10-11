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

local Point3 = _radiant.csg.Point3
local GatherAndCraftAction = class()

GatherAndCraftAction.name = 'gather and craft'
GatherAndCraftAction.does = 'stonehearth:gather_and_craft'
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
   local crafter_component = self._entity:get_component('stonehearth:crafter')
   local workshop = crafter_component:get_workshop()
   local workshop_entity = workshop:get_entity()
   for _, ing_data in ipairs(ingredients) do

      --If the user has paused progress, or cancelled the orderdon't continue.
      --Stopping will cause the  list to be reevaluated from scratch
      if workshop:is_paused() or (workshop:get_curr_order() == nil) then
         return
      end

      local item = ing_data.item
      if not radiant.entities.has_child_by_id(workshop_entity, item:get_id()) then 
         -- grab it!
         ai:execute('stonehearth:pickup_item', ing_data.item)

         -- bring it back!
         ai:execute('stonehearth:goto_entity', workshop_entity)

         -- drop it!!
         ai:execute('stonehearth:run_effect', 'carry_putdown_on_table')
         radiant.entities.add_child(workshop_entity, item, Point3(0, 1, 0))
         entity:get_component('carry_block'):set_carrying(nil)
      end
   end
   -- Once everything's gathered, then craft
   ai:execute('stonehearth:craft')
end

return GatherAndCraftAction
