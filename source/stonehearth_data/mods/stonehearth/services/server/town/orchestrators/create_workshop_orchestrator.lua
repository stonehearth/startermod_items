local CollectIngredients = require 'services.server.town.orchestrators.collect_ingredients_orchestrator'

local CreateWorkshop = class()

function CreateWorkshop:run(town, args)
   local crafter = args.crafter
   local ghost_workshop = args.ghost_workshop
   local outbox_entity = args.outbox_entity

   local workshop_component = ghost_workshop:get_component('stonehearth:ghost_form')
                                                :get_root_entity()
                                                :get_component('stonehearth:workshop')
   local ingredients = workshop_component:get_ingredients()
   local build_sound_effect = workshop_component:get_build_sound_effect()

   self._task_group = town:create_task_group('stonehearth:crafting', {})
                                                  :add_worker(crafter)

   town:run_orchestrator(CollectIngredients, {
      task_group = self._task_group,
      workshop = ghost_workshop,
      ingredients = ingredients
   })

   local args = {
      ghost_workshop = ghost_workshop,
      sound_effect = build_sound_effect,
   }
   local result = town:command_unit(crafter, 'stonehearth:complete_workshop_construction', args)   
                        :once()
                        :start()
                        :wait()
   if not result then
      return
   end

   self:_complete_construction(crafter, ghost_workshop, outbox_entity, args.workshop_task_group)
   return true
end

function CreateWorkshop:stop()
   if self._collector then
      self._collector:destroy()
      self._collector = nil
   end
   if self._task_group then
      self._task_group:destroy()
      self._task_group = nil
   end
end

function CreateWorkshop:_complete_construction(crafter, ghost_workshop, outbox_entity, workshop_task_group)
   local workshop_entity = ghost_workshop:get_component('stonehearth:ghost_form')
                                             :get_root_entity()
   local workshop_component = workshop_entity:get_component('stonehearth:workshop')
   
   -- Place the bench in the world
   local location = radiant.entities.get_world_grid_location(ghost_workshop)
   local q = ghost_workshop:get_component('mob'):get_rotation()

   radiant.terrain.place_entity(workshop_entity, location)
   workshop_entity:get_component('mob'):set_rotation(q)
   workshop_component:finish_construction(workshop_entity, outbox_entity)
   radiant.terrain.remove_entity(ghost_workshop)

   radiant.entities.set_faction(workshop_entity, crafter)
   radiant.entities.set_player_id(workshop_entity, crafter)

   -- destroy the everything we put in the ghost entity to make the workbench
   for id, item in ghost_workshop:add_component('entity_container'):each_child() do
      radiant.entities.destroy_entity(item)
   end

   -- assign the crafter to the workshop and vice versa
   local crafter_component = crafter:get_component('stonehearth:crafter')
   assert(crafter_component, 'crafter has no crafter component while building workshop!')
   crafter_component:set_workshop(workshop_component)
   workshop_component:set_crafter(crafter)
   
   stonehearth.analytics:send_design_event('game:place_workshop', workshop_entity)
end

return CreateWorkshop
