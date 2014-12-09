local Entity = _radiant.om.Entity

local FindStrayAnimal = class()

FindStrayAnimal.name = 'find stray animal'
FindStrayAnimal.does = 'stonehearth:find_stray_animal'
FindStrayAnimal.args = {
   animal = Entity, 
   pasture = Entity   
}
FindStrayAnimal.version = 2
FindStrayAnimal.priority = 1

--Only run if the animal in question is part of a pasture and is not currently following a shepherd
function FindStrayAnimal:start_thinking(ai, entity, args)
   local equipment_component = args.animal:add_component('stonehearth:equipment')
   local pasture_collar = equipment_component:has_item_type('stonehearth:pasture_tag')
   if pasture_collar then
      local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')
      if not shepherded_animal_component:get_following() then
         ai:set_think_output()
      end
   end
end

--every X hours, pasture checks all animals
--if an animal is outside the pasture, the pasture fires this task on the shepherd
--the action ends with the sheep following the shepherd; the return trailing animals to pasture should then kick in

function FindStrayAnimal:run(ai, entity, args)
   ai:execute('stonehearth:drop_carrying_now', {})
   ai:execute('stonehearth:add_buff', {buff = 'stonehearth:buffs:stopped', target = args.animal})
   ai:execute('stonehearth:goto_entity', {entity = args.animal})
   --TODO: can we have a "Holler" animation here, so it looks like the shepherd is calling the critter?

   local equipment_component = args.animal:add_component('stonehearth:equipment')
   local pasture_collar = equipment_component:has_item_type('stonehearth:pasture_tag')
   if pasture_collar then
      local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')
      shepherded_animal_component:set_following(true, entity)
   end

   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.add_trailing_animal then
      shepherd_class:add_trailing_animal(args.animal, args.pasture)
   end
end

return FindStrayAnimal