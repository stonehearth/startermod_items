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
      self._sv.fine_percentage = 0

      local craftable_recipes = {}
      self._sv._recipe_index = {}
      if json.recipe_list then
         craftable_recipes = self:_build_craftable_recipe_list(json.recipe_list)
      end

      -- parts of our save state used by the ui.  careful modifying these     
      self._sv.craftable_recipes = craftable_recipes
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

   local inventory = stonehearth.inventory:get_inventory(self._entity)
   self:_determine_maintain()
   self._added_listener = radiant.events.listen(inventory, 'stonehearth:inventory:stockpile_added', self, self._on_stockpiles_changed)
   self._removed_listener = radiant.events.listen(inventory, 'stonehearth:inventory:stockpile_removed', self, self._on_stockpiles_changed)

end

function CrafterComponent:destroy()
   if self._orchestrator then
      self._orchestrator:destroy()
      self._orchestrator = nil
   end
   local inventory = stonehearth.inventory:get_inventory(self._entity)

   self._added_listener:destroy()
   self._added_listener = nil

   self._removed_listener:destroy()
   self._removed_listener = nil
end

function CrafterComponent:_on_stockpiles_changed()
   self:_determine_maintain()
end

-- True if there are stockpiles, false otherwise. 
-- TODO: consider elaborating to whether the stockpiles can contain stuff crafted by this crafter
function CrafterComponent:_determine_maintain()
   local stockpiles = stonehearth.inventory:get_inventory(self._entity)
                                                :get_all_stockpiles()
   self._sv.should_maintain = next(stockpiles) ~= nil
   self.__saved_variables:mark_changed()
end

function CrafterComponent:should_maintain()
   return self._sv.should_maintain
end

--- Build the list sent to the UI from the json
--  Load each recipe's data and add it to the table
function CrafterComponent:_build_craftable_recipe_list(recipe_index_url)
   self._sv.recipe_index = radiant.resources.load_json(recipe_index_url).craftable_recipes
   local craftable_recipes = {}
   for category, category_data in pairs(self._sv.recipe_index) do
      local recipe_array = {}
      for recipe_name, recipe_data in pairs(category_data) do
         local recipe_data = radiant.resources.load_json(recipe_data.uri)
         self:_initialize_recipe_data(recipe_data)
         table.insert(recipe_array, 1, recipe_data)
      end
      if #recipe_array > 0 then
         --Make an entry in the recipe table for the UI
         local category_ui_info = {}
         category_ui_info.category = category
         category_ui_info.recipes = recipe_array
         --Make sure we have a pointer to the recipe data for fast access/edit'
         category_data.ui_info = category_ui_info
         --table.insert(self._sv.craftable_recipes, category_ui_info)
         table.insert(craftable_recipes, 1, category_ui_info)
      end
   end
   return craftable_recipes
end

--Prep the recipe data with any default values
function CrafterComponent:_initialize_recipe_data(recipe_data)
   --[[
   --Useful for locked recipes (currently not in use, comment out code)
   if recipe_data.locked and recipe_data.requirement then
      if recipe_data.requirement.type == 'unlock_on_make' then
         for i, prerequisite in ipairs(recipe_data.requirement.prerequisites) do
            prerequisite.made = 0
         end
      end
   else
      recipe_data.locked = false
   end
   ]]
   if not recipe_data.level_requirement then
      recipe_data.level_requirement = 0
   end
end


--[[
   Code relevant to whether or not we've unlcoked some recipe based on num made
   Currently not in use for gameplay reasons
]]

-- Add the made parameter to the recipe data

--[[
-- Check to see if any locked recipes are unlocked yet
function CrafterComponent:update_locked_recipes(new_crafted_item_uri)
   for i, category_ui_info in ipairs(self._sv.craftable_recipes) do
      for j, recipe_data in ipairs(category_ui_info.recipes) do
         if recipe_data.locked and recipe_data.requirement and recipe_data.requirement.type == 'unlock_on_make' then
            for i, prerequisite in ipairs(recipe_data.requirement.prerequisites) do
               if prerequisite.item == new_crafted_item_uri then
                  prerequisite.made = prerequisite.made + 1
                  if prerequisite.made == prerequisite.required then
                     self:_test_for_unlock(recipe_data)
                  end
               end
            end
         end
      end
   end
   self.__saved_variables:mark_changed()
end


function CrafterComponent:_test_for_unlock(recipe_data)
   local unlock = true
   if recipe_data.locked and recipe_data.requirement then
      if recipe_data.requirement.type == 'unlock_on_make' then
         for i, prerequisite in ipairs(recipe_data.requirement.prerequisites) do
            if prerequisite.made < prerequisite.required then
               unlock = false
               break
            end
         end
      end
   end
   if unlock then
      recipe_data.locked = false
   end
end
]]
--[[End code relevant to whether or not we've unlcoked some recipe based on num made ]]

--[[
--TODO: test this with a scenario
--- Add a recipe to a category, and add it to the UI.  
--  If the category doesn't yet exist, create a new one.
--  @param category - the name of the category
--  @param recipe_uri - the uri to the recipe
function CrafterComponent:add_recipe(category, recipe_uri)
   --TODO: make new category, if necessary
   local category_data = self._sv._recipe_index[category]
   if not category_data then
      category_data = {
         recipes = [],
      }
      self._sv._recipe_index[category] = category_data
   end

   --If there is no UI category for this yet, make one
   local ui_info = category_data.category_ui_info
   if not ui_info then
      ui_info = {
         category = category, 
         recipes = {}
      }
      category_data.category_ui_info = ui_info
      table.insert(self._sv.craftable_recipes, ui_info)
   end

   local recipe_data = radiant.resources.load_json(recipe_uri)
   table.insert(ui_info.recipes, recipe_data)

   self.__saved_variables:mark_changed()
end
--]]

--- Returns the effect to play as the crafter works
function CrafterComponent:get_work_effect()
   return self._sv.work_effect
end

function CrafterComponent:create_workshop(ghost_workshop)
   self:_create_workshop_orchestrator(ghost_workshop)
end

function CrafterComponent:_create_workshop_orchestrator(ghost_workshop)
   -- create a task group for the workshop.  we'll use this both to build it and
   -- to feed the crafter orders when it's finally created
   local town = stonehearth.town:get_town(self._entity)
   self._orchestrator = town:create_orchestrator(CreateWorkshop, {
      crafter = self._entity,
      ghost_workshop = ghost_workshop
   })
   self._sv._create_workshop_args = {
      ghost_workshop = ghost_workshop
   }
   self.__saved_variables:mark_changed()
end

-- Let the crafter know which workshop component is his
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

--Returns the workshop component associated with this crafter
function CrafterComponent:get_workshop()
   return self._sv.workshop
end

-- Unset the workshop
-- Presumes that the caller calls remove_component afterwards; this class will not be reused
function CrafterComponent:demote()
   if self._sv.workshop then
      self._sv.workshop:set_crafter(nil)
   end
end

--Set the chances that the crafter will make something fine
function CrafterComponent:set_fine_percentage(percentage)
   self._sv.fine_percentage = percentage
end

--Get the chances that the crafter will make something fine
--Nil until the crafter reaches a certain level. 
function CrafterComponent:get_fine_percentage()
   return self._sv.fine_percentage
end

-- Given a talisman, associate it with our workshop, if we have one
function CrafterComponent:associate_talisman_with_workshop(talisman_entity)
   local talisman_component = talisman_entity:get_component('stonehearth:promotion_talisman')
   if not talisman_component then
      return
   end
   if self._sv.workshop then
      talisman_component:associate_with_entity('workshop_component', self._sv.workshop:get_entity() )
   end
end


--if this talisman is associated with an existing workshop, we should use that workshop
--instead of making a new one. 
-- @returns the associated workshop entity
function CrafterComponent:setup_with_existing_workshop(talisman_entity)
   if not talisman_entity then 
      return
   end

   local talisman_component = talisman_entity:get_component('stonehearth:promotion_talisman')
   if not talisman_component then
      return
   end

   local associated_workshop = talisman_component:get_associated_entity('workshop_component')
   if associated_workshop then
      local workshop_component = associated_workshop:get_component('stonehearth:workshop')
      self:set_workshop(workshop_component)
      workshop_component:set_crafter(self._entity)
      return associated_workshop
   end
end

return CrafterComponent
