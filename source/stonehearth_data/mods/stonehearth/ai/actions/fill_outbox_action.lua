--[[
   After an item has been crafted, add its outputs to the outbox.
   TODO: Add the small version of each item
   TODO: Read the appropriate outbox location from the crafter's class_info.json
   TODO: Make the outbox an actual entity container?
   TODO: If there are side-effect products, like toxic waste, handle separately
]]
local Point3 = _radiant.csg.Point3
local FillOutboxAction = class()

FillOutboxAction.name = 'fill outbox'
FillOutboxAction.does = 'stonehearth:fill_outbox'
FillOutboxAction.version = 1
FillOutboxAction.priority = 5

function FillOutboxAction:__init(ai, entity)
end

--[[
   All the outputs should be created by now.
   Run over to the outbox and put all the outputs in it.
   TODO: crafter really should carry the small versions of objects; the big versions have several issues
   --model is created to be worn off center (buckler)
   --model realigns itself upon being placed into the world
]]
function FillOutboxAction:run(ai, entity)
   local crafter_component = entity:get_component('stonehearth:crafter')
   local workshop = crafter_component:get_workshop()
   local workshop_entity = workshop:get_entity()
   local outbox_entity = workshop:get_outbox_entity()

   while not workshop:is_paused() and workshop:has_bench_outputs() do
      -- take the item off the workbench
      local item = workshop:pop_bench_output()
      ai:execute('stonehearth:pickup_item_on_table', item, workshop_entity)

      -- drop it in the outbox...
      local pathfinder = radiant.pathfinder.create_path_finder(entity, 'goto entity action')
                              :add_destination(outbox_entity)
      local path = ai:wait_for_path_finder(pathfinder)
      local drop_location = path:get_destination_point_of_interest()

      ai:execute('stonehearth:follow_path', path)
      ai:execute('stonehearth:drop_carrying', drop_location)
   end
end

return FillOutboxAction