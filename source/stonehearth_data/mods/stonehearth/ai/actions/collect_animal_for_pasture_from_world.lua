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
-- and as long as there are no unclaimed sheep nearby,
-- wander into the world (and away from town)
-- TODO: wander into the part of the world that is appropriate for this animal type
-- Generate an animal of the appropriate type near me
-- At this point the other, higher-priority "collect animal for pasture from nearby" should take over
-- TODO: refactor so that these guys listen on num animals, and react thereby (see pattern from return animals to pasture)
-- Current implementation works but relies on AI getting reset by higher priority items
function CollectAnimalFromWorld:start_thinking(ai, entity, args)
   local pasture_component = args.pasture:get_component('stonehearth:shepherd_pasture')
   if not self._timer and 
      pasture_component and 
      pasture_component:get_num_animals() < pasture_component:get_min_animals() then
      
      --Wait n min world-time so that if there are existing animals in the world nearby, we
      --have a chance to get to them. If not, run this
      --When the timer finishes, it sets a bit; on next execution, will get the critter.
      self._timer = stonehearth.calendar:set_timer('5m', function()
         self._timer = nil
         --self._ready_to_run = true
         ai:set_think_output()
      end)

      --if self._ready_to_run then
      --   ai._ready_to_run = false
      --   ai:set_think_output()
      --end
   end
end

function CollectAnimalFromWorld:stop_thinking()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
end

--TODO: put these magic numbers somewhere
function CollectAnimalFromWorld:run(ai, entity, args)

   local pasture_component = args.pasture:get_component('stonehearth:shepherd_pasture')
   if pasture_component then
      -- wander a bunch
      ai:execute('stonehearth:drop_carrying_now', {})
      ai:execute('stonehearth:wander', {radius = 100, radius_min = 30})
      ai:execute('stonehearth:run_effect', {effect = 'idle_look_around'})

      local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
      if shepherd_class and shepherd_class.can_find_animal_in_world then
         if shepherd_class:can_find_animal_in_world() then
            -- pop an animal near where we've wandered
            local origin = radiant.entities.get_world_grid_location(entity)
            local placementPoint = radiant.terrain.find_placement_point(origin, 1, 20)
            if placementPoint then
               local animal_type = pasture_component:get_pasture_type()
               local animal = radiant.entities.create_entity(animal_type)
               radiant.terrain.place_entity(animal, placementPoint)
            end
         end
      end
   end
   --At this point, hopefully the other function should take over
   --If not, go to critter, and tame it
end

function CollectAnimalFromWorld:stop(ai, entity, args)
   assert(true)
end

return CollectAnimalFromWorld 