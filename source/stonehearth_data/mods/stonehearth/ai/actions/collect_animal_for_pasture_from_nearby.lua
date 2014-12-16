local Entity = _radiant.om.Entity
local CollectAnimalFromNearby = class()

CollectAnimalFromNearby.name = 'collect animal for pasture from nearby'
CollectAnimalFromNearby.status_text = 'claiming animal'
CollectAnimalFromNearby.does = 'stonehearth:collect_animals_for_pasture'
CollectAnimalFromNearby.args = {
   pasture = Entity   --the pasture entity that yearns for this animal
}
CollectAnimalFromNearby.version = 2
CollectAnimalFromNearby.priority = 2  --higher priority than wandering into the world to find a new critter

ALL_FILTER_FNS = {}

-- In order to start, the pasture must not be full and  
-- there should be a free animal of the appropriate type nearby
-- TODO: refactor so that these guys listen on num animals, and react thereby
function CollectAnimalFromNearby:start_thinking(ai, entity, args)
   local pasture_component = args.pasture:get_component('stonehearth:shepherd_pasture')

   --Check that the pasture is not full
   --if pasture_component and pasture_component:get_num_animals() < pasture_component:get_max_animals() then
      
   --Check that there is a nearby animal of the correct type
   local animal_type = pasture_component:get_pasture_type()
   local filter_fn = ALL_FILTER_FNS[animal_type]
   if not filter_fn then
      filter_fn = self:make_filter_fn(animal_type)
   end

   ai:set_think_output({
      filter_fn = filter_fn,
      description = animal_type
      })
   --end
end

--The filter fn should check if the animal is of the right type
--and also if it doesn't have the pasture collar equipment item
function CollectAnimalFromNearby:make_filter_fn(animal_type)
   local filter_fn = function(entity) 
      if entity:get_uri() == animal_type then
         local equipment_component = entity:get_component('stonehearth:equipment')
         if not equipment_component then
            return true
         elseif equipment_component and not equipment_component:has_item_type('stonehearth:pasture_tag') then
            return true
         end
      end 
      return false
   end
   return filter_fn
end

--TODO: put this range into a constant?

local ai = stonehearth.ai
return ai:create_compound_action(CollectAnimalFromNearby)
   :execute('stonehearth:wait_for_pasture_vacancy', {pasture = ai.ARGS.pasture})
   :execute('stonehearth:drop_carrying_now', {})
   :execute('stonehearth:find_path_to_entity_type', {
            filter_fn = ai.BACK(3).filter_fn,
            description = ai.BACK(3).description,
            range = 30
         })
   :execute('stonehearth:add_buff', {buff = 'stonehearth:buffs:stopped', target = ai.PREV.path:get_destination()})
   :execute('stonehearth:follow_path', {
            path = ai.BACK(2).path,
         })
   :execute('stonehearth:reserve_entity', { entity = ai.BACK(3).path:get_destination() })
   :execute('stonehearth:turn_to_face_entity', { entity = ai.BACK(4).path:get_destination() })
   :execute('stonehearth:run_effect', { effect = 'whistle' })
   :execute('stonehearth:claim_animal_for_pasture', {pasture = ai.ARGS.pasture, animal = ai.BACK(6).path:get_destination()})