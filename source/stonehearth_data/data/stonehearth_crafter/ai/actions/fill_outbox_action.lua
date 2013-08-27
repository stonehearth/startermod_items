--[[
   After an item has been crafted, add its outputs to the outbox.
   TODO: Add the small version of each item
   TODO: Read the appropriate outbox location from the crafter's class_info.json
   TODO: Make the outbox an actual entity container?
   TODO: If there are side-effect products, like toxic waste, handle separately
]]
local Point3 = _radiant.csg.Point3
local FillOutboxAction = class()

FillOutboxAction.name = 'stonehearth.actions.fill_outbox'
FillOutboxAction.does = 'stonehearth_crafter.activities.fill_outbox'
FillOutboxAction.priority = 5

function FillOutboxAction:__init(ai, entity)
   self._curr_carry = nil
end

--[[
   All the outputs should be created by now.
   Run over to the outbox and put all the outputs in it.
   TODO: crafter really should carry the small versions of objects; the big versions have several issues
   --model is created to be worn off center (buckler)
   --model realigns itself upon being placed into the world
]]
function FillOutboxAction:run(ai, entity)
   local crafter_component = entity:get_component('stonehearth_crafter:crafter')
   local workshop = crafter_component:get_workshop()
   local workshop_entity = workshop:get_entity()
   self._outbox_entity = workshop:get_outbox()

   assert(workshop:has_bench_outputs(), 'Trying to fill outbox, but the bench has no products!')

    repeat
      self._curr_carry = workshop:pop_bench_output()

      --TODO: pickup from table?
      ai:execute('stonehearth.activities.pickup_item', self._curr_carry)

      --Find path
      --Something like:
      local path = nil
      local solved = function(solution)
         path = solution
      end

      local pathfinder = native:create_path_finder('goto entity action', entity, solved, nil)
      pathfinder:add_destination(self._outbox_entity)

      ai:wait_until(function()
         return path ~= nil
      end)
      ai:execute('stonehearth.activities.follow_path', path)
      local drop_location = path:get_finish_point()
      ai:execute('stonehearth.activities.drop_carrying', drop_location)
      self._curr_carry = nil

      --If the user has paused progress, don't continue
      if workshop:is_paused() then
         return
      end

   until not workshop:has_bench_outputs()


end

--[[
   If interrupted, while moving between the workbench and outbox,
   pitch the item into the outbox before stopping.
   TODO: is this the right way to programatically add something to the outbox?
   TODO: can the outbox expose an "add to me" function?
]]
function FillOutboxAction:stop()
   if self._curr_carry and self._outbox_entity then
      radiant.entities.add_child(self._outbox_entity, self._curr_carry, Point3(0, 1, 0))
      self._curr_carry = nil
   end
end

return FillOutboxAction