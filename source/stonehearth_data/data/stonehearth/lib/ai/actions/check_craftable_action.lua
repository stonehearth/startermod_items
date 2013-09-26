--[[
   At some interval, check if the crafter should be crafting anything.
   If the worker's todo list has something that can be
   crafted right now, or if the last thing we tried to craft is waiting
   unfinished ont he bench, change the priority of this component to be
   higher (5) and call the appropriate gather or craft action.
]]

local CheckCraftableAction = class()

CheckCraftableAction.name = 'stonehearth.actions.check_craftable'
CheckCraftableAction.does = 'stonehearth.activities.top'
CheckCraftableAction.priority = 0

function CheckCraftableAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         --the crafter
   self._ai = ai

   self._is_crafting = false     --whether we're currently actively crafting something
   self._next_activity = {}      --the thing we shoud be doing next (gathering/crafting)

   radiant.events.listen('radiant.events.gameloop', self)
end

--[[
   On gameloop (TODO: replace with on inventory added, or something)
   check if the worker associated with this action:

   a.) is currently in the middle of crafting something
   b.) was crafting but got interrupted
   c.) can craft something new off the todo list

   If a.), return.
   If b.) tell the worker to resume crafting.
   If c.), start gathering materials for crafting.
]]
CheckCraftableAction['radiant.events.gameloop'] = function(self)
   --TODO: Fix unlisten because it's not immediately de-registering us
   if self._is_crafting then
      return
   end

   assert(not self._is_crafting,"Whoa! We're crafting? Stop the presses")

   local crafter_component = self._entity:get_component('stonehearth:crafter')
   local workshop = crafter_component:get_workshop()

   --TODO: Do some validation here, and then call self._ai:abort() when it's implemented
   --For now, just return out
   if not (crafter_component and workshop) then
      return
   end

   --If the user has paused progress, don't continue
   if workshop:is_paused() then
      return
   end

   local new_priority = 0

   if workshop:is_currently_crafting() then
      self._next_activity = {'stonehearth_crafter.activities.craft'}
      new_priority = 5
   elseif workshop:has_bench_outputs() then
      self._next_activity = {'stonehearth_crafter.activities.fill_outbox'}
      new_priority = 5
   else
      local recipe, ingredient_data = workshop:establish_next_craftable_recipe()
      if recipe then
         self._next_activity = {'stonehearth_crafter.activities.gather_and_craft', recipe, ingredient_data}
         new_priority = 5
      end
   end
   self._ai:set_action_priority(self, new_priority)
end

--[[
   When the action comes up on the priority lottery,
   call the actual craft action with the next activity
   and relevant arguments
]]
function CheckCraftableAction:run(ai, entity)
   radiant.events.unlisten('radiant.events.gameloop', self)
   self._is_crafting = true
   ai:execute(unpack(self._next_activity))
end

--[[
   Always called after run, or if the action is interrupted
   If any of the actions spawned by CheckCraftable are
   stopped, be sure to remember that we're not currently
   crafting (so the next check can act appopriately)
]]
function CheckCraftableAction:stop()
   self._is_crafting = false
   self._ai:set_action_priority(self, 0)
   radiant.events.listen('radiant.events.gameloop', self)
end

return CheckCraftableAction