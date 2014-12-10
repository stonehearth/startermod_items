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

local ai = stonehearth.ai
return ai:create_compound_action(FindStrayAnimal)
   :execute('stonehearth:drop_carrying_now', {})
   :execute('stonehearth:add_buff', {buff = 'stonehearth:buffs:stopped', target = ai.ARGS.animal})
   :execute('stonehearth:goto_entity', {entity = ai.ARGS.animal})
   :execute('stonehearth:reserve_entity', { entity = ai.ARGS.animal })
   :execute('stonehearth:turn_to_face_entity', { entity =  ai.ARGS.animal })
   :execute('stonehearth:run_effect', { effect = 'fiddle' })
   :execute('stonehearth:claim_animal_for_pasture', {pasture = ai.ARGS.pasture, animal = ai.ARGS.animal})

