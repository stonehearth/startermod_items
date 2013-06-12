--[[
   After an item has been crafted, add its outputs to the outbox.
   TODO: Add the small version of each item
   TODO: Read the appropriate outbox location from the crafter's class_info.txt
   TODO: Make the outbox an actual entity container?
   TODO: If there are side-effect products, like toxic waste, handle separately
]]
local FillOutboxAction = class()

FillOutboxAction.name = 'stonehearth.actions.fill_outbox'
FillOutboxAction.does = 'stonehearth_crafter.activities.fill_outbox'
FillOutboxAction.priority = 5

function FillOutboxAction:__init(ai, entity)
   self._curr_carry = nil
   self._output_x = -0
   self._output_y = -0
end

--[[
   All the outputs should be created by now.
   Run over to the outbox and put all the outputs in it.
   TODO: crafter really should carry the small versions of objects; the big versions have several issues
   --model is created to be worn off center (buckler)
   --model realigns itself upon being placed into the world
]]
function FillOutboxAction:run(ai, entity)
   local crafter_component = entity:get_component('mod://stonehearth_crafter/components/crafter.lua')
   local workshop = crafter_component:get_workshop()

   assert(workshop:has_bench_outputs(), 'Trying to fill outbox, but the bench has no products!')

   repeat
      local bench_location = RadiantIPoint3(-12, 1, -11)
      self._curr_carry = workshop:pop_bench_output()
      local goto = RadiantIPoint3(self._output_x, 1, self._output_y)
      self._output_x = self._output_x + 1
      ai:execute('stonehearth.activities.bring_to', self._curr_carry, goto)
      self._curr_carry = nil
   until not workshop:has_bench_outputs()
end

--[[
   If interrupted, while moving between the workbench and outbox, pitch the item into the outbox
   before stopping.
]]
function FillOutboxAction:stop()
   if self._curr_carry then
      radiant.terrain.place_entity(self._curr_carry, RadiantIPoint3(self._output_x + 1, 1, self._output_y))
   end
end

return FillOutboxAction