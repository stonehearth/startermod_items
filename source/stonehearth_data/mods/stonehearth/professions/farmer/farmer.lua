local farmer_class = {}

function farmer_class.promote(entity)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:farmer:profession_description')
   local profession_component = entity:add_component('stonehearth:profession', profession_description.profession)
   
   --Slap a new outfit on the farmer
   local equipment = entity:add_component('stonehearth:equipment')

   --TODO: change this into a farmer outfit
   equipment:equip_item('stonehearth:farmer:outfit')

   --Add them to the farmer scheduler
   local faction = radiant.entities.get_faction(entity)
   local town = stonehearth.town:get_town(faction)
   town:join_task_group(entity, 'farmers')
end

function farmer_class.demote(entity)

   local faction = radiant.entities.get_faction(entity)
   local town = stonehearth.town:get_town(faction)
   town:leave_task_group(entity, 'farmers')

   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')

   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
end

return farmer_class
