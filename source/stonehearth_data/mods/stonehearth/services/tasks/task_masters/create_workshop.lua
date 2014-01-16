local CreateWorkshop = class()
local IngredientList = require 'components.workshop.ingredient_list'

function CreateWorkshop:__init(ghost_entity, outbox_entity)
   self._ghost_entity = ghost_entity
   self._outbox_entity = outbox_entity
   self._faction = radiant.entities.get_faction(ghost_entity)
   
   local json = self._ghost_entity:get_component('stonehearth:ghost_item'):get_full_sized_json()
   local ingredients = json.components['stonehearth:workshop'].ingredients
   self._build_sound_effect = json.components['stonehearth:workshop'].build_sound_effect
   self._ingredients = IngredientList(ingredients)
   self:_gather_next_construction_ingredient()
end

function CreateWorkshop:_gather_next_construction_ingredient()
   -- gather the ingredients one by one
   if self._ingredients:completed() then
      -- yay!!
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
            return radiant.events.UNLISTEN
         end)
   else
      local next = self._ingredients:get_next_ingredient()
      local name = string.format('get %s for workshop', next.material)
      local args = {
         material = next.material,
         workshop = self._ghost_entity,
         ingredient_list = self._ingredients,
      }
      local task = stonehearth.tasks:get_scheduler('stonehearth:workers', self._faction)
                                         :create_task('stonehearth:collect_ingredient', args)
                                         :set_name(name)
                                         :once()
                                         :start()

      radiant.events.listen(task, 'completed', function()
            self:_gather_next_construction_ingredient()
            return radiant.events.UNLISTEN
         end)
   end
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
