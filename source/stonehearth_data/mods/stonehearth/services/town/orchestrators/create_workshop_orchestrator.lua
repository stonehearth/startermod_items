local CollectIngredients = require 'services.town.orchestrators.collect_ingredients_orchestrator'
local WorkAtWorkshop = require 'services.town.orchestrators.work_at_workshop_orchestrator'

local CreateWorkshop = class()

function CreateWorkshop:run(town, args)
   local crafter = args.crafter
   local ghost_workshop = args.ghost_workshop
   local outbox_entity = args.outbox_entity
   local task_group = args.task_group

   local json = ghost_workshop:get_component('stonehearth:ghost_item'):get_full_sized_json()
   local workshop_data = json.components['stonehearth:workshop']

   town:run_orchestrator(CollectIngredients, {
      task_group = task_group,
      workshop = ghost_workshop,
      ingredients = workshop_data.ingredients
   })

   local args = {
      ghost_workshop = ghost_workshop,
      sound_effect = workshop_data.build_sound_effect,
   }
   local result = town:command_unit(crafter, 'stonehearth:complete_workshop_construction', args)   
                        :once()
                        :start()
                        :wait()
   if not result then
      return
   end

   local workshop = self:_complete_construction(crafter, ghost_workshop, outbox_entity, args.workshop_task_group)

   -- fire put the orchestrator
   town:create_orchestrator(WorkAtWorkshop, {
         crafter = crafter,
         task_group  = task_group,
         workshop = workshop:get_entity(),
         craft_order_list = workshop:get_craft_order_list(),         
      })
      
   return true
end

function CreateWorkshop:stop()
   if self._collector then
      self._collector:destroy()
      self._collector = nil
   end
end

function CreateWorkshop:_complete_construction(crafter, ghost_workshop, outbox_entity, workshop_task_group)
   local faction = radiant.entities.get_faction(crafter)
   local real_item_uri = ghost_workshop:get_component('stonehearth:ghost_item'):get_full_sized_mod_uri();

   local workshop_entity = radiant.entities.create_entity(real_item_uri)
   local workshop_component = workshop_entity:get_component('stonehearth:workshop')
   
   -- Place the bench in the world
   local location = radiant.entities.get_world_grid_location(ghost_workshop)
   local q = ghost_workshop:get_component('mob'):get_rotation()
   radiant.terrain.place_entity(workshop_entity, location)
   workshop_entity:get_component('mob'):set_rotation(q)
   workshop_component:finish_construction(faction, outbox_entity)

   -- destroy the ghost entity and everything we put in it to make the workbench
   for id, item in ghost_workshop:add_component('entity_container'):each_child() do
      radiant.entities.destroy_entity(item)
   end
   radiant.entities.destroy_entity(ghost_workshop)

   -- assign the crafter to the workshop
   workshop_component:set_crafter(crafter)
   
   stonehearth.analytics:send_design_event('game:place_workshop', workshop_entity)
   
   return workshop_component
end

return CreateWorkshop
