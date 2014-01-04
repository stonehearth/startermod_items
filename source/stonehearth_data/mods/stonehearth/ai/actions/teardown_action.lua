local Fabricate = class()

Fabricate.does = 'stonehearth:teardown'
Fabricate.version = 1
Fabricate.priority = 5

function Fabricate:run(ai, entity, path)
   local endpoint = path:get_destination()
   local fabricator = endpoint:get_component('stonehearth:fabricator')
   local block = path:get_destination_point_of_interest()
   
   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, block)
   ai:execute('stonehearth:run_effect', 'work')

   if fabricator:remove_block(block) then
      if not radiant.entities.increment_carrying(entity) then
         local oak_log = radiant.entities.create_entity('stonehearth:oak_log')
         local item_component = oak_log:add_component('item')
         item_component:set_stacks(1)
         radiant.entities.pickup_item(entity, oak_log)
      end
   end
end

return Fabricate
