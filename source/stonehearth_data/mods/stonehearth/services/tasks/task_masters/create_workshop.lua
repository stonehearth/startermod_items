local CollectIngredients = require 'services.tasks.task_masters.collect_ingredients_task_master'
local CreateWorkshop = class()

function CreateWorkshop:__init(scheduler, ghost_entity, outbox_entity, faction)
   self._faction = faction
   self._scheduler = scheduler
   self._ghost_entity = ghost_entity
   self._outbox_entity = outbox_entity
   
   local json = self._ghost_entity:get_component('stonehearth:ghost_item'):get_full_sized_json()
   local ingredients = json.components['stonehearth:workshop'].ingredients
   self._build_sound_effect = json.components['stonehearth:workshop'].build_sound_effect

   self._gather_task_master = CollectIngredients(scheduler, ingredients, ghost_entity)
end

function CreateWorkshop:start()
   radiant.events.listen(self._gather_task_master, 'completed', function()
         self:_start_complete_construction_task()
         return radiant.events.UNLISTEN
      end)

   self._gather_task_master:start()
end

function CreateWorkshop:_start_complete_construction_task()
   local task = stonehearth.tasks:get_scheduler('stonehearth:workers', self._faction)
                                      :create_task('stonehearth:complete_workshop_construction', {
                                          ghost_workshop = self._ghost_entity,
                                          sound_effect = self._build_sound_effect,
                                      })
                                      :set_name('finishing workshop')
                                      :once()
                                      :start()
   radiant.events.listen(task, 'completed', function()
         self:_complete_construction()
         radiant.events.trigger(self, 'completed')
         return radiant.events.UNLISTEN
      end)
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

   --destroy the ghost entity
   radiant.entities.destroy_entity(self._ghost_entity)

   stonehearth.analytics:send_design_event('game:place_workshop', workshop_entity)
end

return CreateWorkshop
