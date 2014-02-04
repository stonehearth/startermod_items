local CreateWorkshop = class()

function CreateWorkshop:start(scheduler, args)
   self._faction = args.faction
   self._ghost_entity = args.ghost_workshop
   self._outbox_entity = args.outbox_entity
   
   local json = self._ghost_entity:get_component('stonehearth:ghost_item'):get_full_sized_json()
   self._ingredients = json.components['stonehearth:workshop'].ingredients
   self._build_sound_effect = json.components['stonehearth:workshop'].build_sound_effect
end

function CreateWorkshop:run(thread, args)
   thread:orchestrate('stonehearth:tasks:collect_ingredients', {
      workshop = self._ghost_entity,
      ingredients = self._ingredients,
   })
   thread:execute('stonehearth:complete_workshop_construction', {
      ghost_workshop = self._ghost_entity,
      sound_effect = self._build_sound_effect,
   })
   self:_complete_construction()
end

function CreateWorkshop:_complete_construction()
   local real_item_uri = self._ghost_entity:get_component('stonehearth:ghost_item'):get_full_sized_mod_uri();

   local workshop_entity = radiant.entities.create_entity(real_item_uri)
   local workshop_component = workshop_entity:get_component('stonehearth:workshop')
   
   -- Place the bench in the world
   local location = radiant.entities.get_world_grid_location(self._ghost_entity)
   local q = self._ghost_entity:get_component('mob'):get_rotation()
   radiant.terrain.place_entity(workshop_entity, location)
   workshop_entity:get_component('mob'):set_rotation(q)
   workshop_component:finish_construction(self._faction, self._outbox_entity)

   -- destroy the ghost entity and everything we put in it to make the workbench
   for id, item in self._ghost_entity:add_component('entity_container'):each_child() do
      radiant.entities.destroy_entity(item)
   end
   radiant.entities.destroy_entity(self._ghost_entity)

   stonehearth.analytics:send_design_event('game:place_workshop', workshop_entity)
end

return CreateWorkshop
