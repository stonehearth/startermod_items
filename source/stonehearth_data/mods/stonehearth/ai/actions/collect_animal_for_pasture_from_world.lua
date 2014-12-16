local Entity = _radiant.om.Entity
local CollectAnimalFromWorld = class()

CollectAnimalFromWorld.name = 'collect animal for pasture from world'
CollectAnimalFromWorld.status_text = 'looking for animals'
CollectAnimalFromWorld.does = 'stonehearth:collect_animals_for_pasture'
CollectAnimalFromWorld.args = {
   pasture = Entity   --the pasture entity that yearns for this animal
}
CollectAnimalFromWorld.version = 2
CollectAnimalFromWorld.priority = 1

-- As long as we haven't hit the min number of animals for this pasture,
-- and as long as there are no unclaimed animals of the right type nearby,
-- wander into the world (and away from town) and generate an animal of the appropriate type
-- TODO: wander into the part of the world that is appropriate for this animal type

local ai = stonehearth.ai
return ai:create_compound_action(CollectAnimalFromWorld)
   :execute('stonehearth:wait_for_pasture_vacancy', {pasture = ai.ARGS.pasture, wait_timeout = '5m'})
   :execute('stonehearth:create_animal_for_pasture', {pasture = ai.ARGS.pasture})
   :execute('stonehearth:drop_carrying_now', {})
   :execute('stonehearth:goto_entity', {entity = ai.BACK(2).animal})
   :execute('stonehearth:run_effect', {effect = 'idle_look_around'})
