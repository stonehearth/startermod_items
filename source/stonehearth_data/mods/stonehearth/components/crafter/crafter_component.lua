--[[
   Crafter.lua implements all functionality associated with a
   crafter citizen. 
]]

local CrafterComponent = class()
local CreateWorkshop = require 'services.server.town.orchestrators.create_workshop_orchestrator'

function CrafterComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      -- xxx: a lot of this is read-only data.  can we store the uri to that data instead of putting
      -- it in the crafter component?  then we don't have to stick it in sv!
      self._sv._initialized = true
      self._sv.work_effect = json.work_effect

      local craftable_recipes = {}
      self._sv._recipe_index = {}
      if json.recipe_list then
         craftable_recipes = self:_build_craftable_recipe_list(json.recipe_list)
      end

      -- parts of our save state used by the ui.  careful modifying these     
      self._sv.craftable_recipes = craftable_recipes

      self._sv.active = false
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
            local o = self._sv._create_workshop_args
            if o then
               self:_create_workshop_orchestrator(o.ghost_workshop, o.outbox_entity)
            end
         end)
   end
   -- what about the orchestrator?  hmm...  will the town restore it for us?
   -- how do we get a pointer to it so we can destroy it?
end

function CrafterComponent:destroy()
   if self._orchestrator then
      self._orchestrator:destroy()
      self._orchestrator = nil
   end
end

--- Build the list sent to the UI from the json
--  TODO: add unlock conditions to the recipes
function CrafterComponent:_build_craftable_recipe_list(recipe_index_url)
   self._sv.recipe_index = radiant.resources.load_json(recipe_index_url).craftable_recipes
   local craftable_recipes = {}
   for category, category_data in pairs(self._sv.recipe_index) do
      local recipe_array = {}
      for recipe_name, data in pairs(category_data) do
         --If there's no unlock condition, then just put in the recipe
         if not data.unlock_condition then
            table.insert(recipe_array, 1, data.uri)
         end
      end
      if #recipe_array > 0 then
         --Make an entry in the recipe table for the UI
         local category_ui_info = {}
         category_ui_info.categrory = category
         category_ui_info.recipes = recipe_array
         --Make sure we have a pointer to the recipe data for fast access/edit'
         category_data.ui_info = category_ui_info
         --table.insert(self._sv.craftable_recipes, category_ui_info)
         table.insert(craftable_recipes, 1, category_ui_info)
      end
   end
   return craftable_recipes
end

--TODO: test this with a scenario
--- Add a recipe to a category, and add it to the UI.  
--  If the category doesn't yet exist, create a new one.
--  @param category - the name of the category
--  @param recipe_uri - the uri to the recipe
--  @param recipe_data - data about when to activate the recipe
function CrafterComponent:add_recipe(category, recipe_uri, recipe_data)
   --TODO: make new category, if necessary
   local category_data = self._sv._recipe_index[category]
   if not category_data then
      category_data = {
         recipes = {},
      }
      self._sv._recipe_index[category] = category_data
   end
   category_data.recipes[recipe_uri] = recipe_data

   --If there is no unlock condition then add the recipe to the UI structure
   if not recipe_data.unlock_condition then
      --If there is no UI category for this yeat, make one
      local ui_info = category_data.category_ui_info
      if not ui_info then
         ui_info = {
            category = category, 
            recipes = {}
         }
         category_data.category_ui_info = ui_info
         table.insert(self._sv.craftable_recipes, ui_info)
      end
      table.insert(ui_info.recipes, recipe_uri)
   end
   self.__saved_variables:mark_changed()
end


--- Returns the effect to play as the crafter works
function CrafterComponent:get_work_effect()
   return self._sv.work_effect
end

function CrafterComponent:create_workshop(ghost_workshop, outbox_location, outbox_size)
   local faction = radiant.entities.get_faction(self._entity)
   local outbox_entity = radiant.entities.create_entity('stonehearth:workshop_outbox')
   radiant.terrain.place_entity(outbox_entity, outbox_location)
   
   radiant.entities.set_faction(outbox_entity, self._entity)
   radiant.entities.set_player_id(outbox_entity, self._entity)

   local outbox_component = outbox_entity:get_component('stonehearth:stockpile')
   outbox_component:set_size(outbox_size.x, outbox_size.y)
   outbox_component:set_outbox(true)

   self:_create_workshop_orchestrator(ghost_workshop, outbox_entity)
end

function CrafterComponent:_create_workshop_orchestrator(ghost_workshop, outbox_entity)
   -- create a task group for the workshop.  we'll use this both to build it and
   -- to feed the crafter orders when it's finally created
   local town = stonehearth.town:get_town(self._entity)
   self._orchestrator = town:create_orchestrator(CreateWorkshop, {
      crafter = self._entity,
      ghost_workshop = ghost_workshop,
      outbox_entity = outbox_entity,
   })
   self._sv._create_workshop_args = {
      ghost_workshop = ghost_workshop,
      outbox_entity = outbox_entity,
   }
   self.__saved_variables:mark_changed()
end

function CrafterComponent:set_workshop(workshop_component)
   self._sv._create_workshop_args = nil
   self.__saved_variables:mark_changed()

   if workshop_component ~= self._sv.workshop then
      self._sv.workshop = workshop_component
      self.__saved_variables:mark_changed()

      radiant.events.trigger_async(self._entity, 'stonehearth:crafter:workshop_changed', {
            entity = self._entity,
            workshop = self._sv.workshop,
         })
   end
end

return CrafterComponent
