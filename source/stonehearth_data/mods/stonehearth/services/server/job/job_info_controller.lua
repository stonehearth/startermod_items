local JobInfoController = class()

function JobInfoController:initialize(info)
   if info.description then
      local desc = radiant.resources.load_json(info.description)
      if desc then
         local crafter = desc.crafter
         if crafter then
            if crafter.recipe_list then
               self:_build_craftable_recipe_list(crafter.recipe_list)
            end
         end
      end
   end
   self.__saved_variables:mark_changed()
end

--- Build the list sent to the UI from the json
--  Load each recipe's data and add it to the table
function JobInfoController:_build_craftable_recipe_list(recipe_index_url)
   self._sv.recipe_list = radiant.resources.load_json(recipe_index_url).craftable_recipes

   local craftable_recipes = {}
   for category, category_data in pairs(self._sv.recipe_list) do
      for recipe_name, recipe_data in pairs(category_data.recipes) do
         recipe_data.recipe = radiant.resources.load_json(recipe_data.recipe)
         self:_initialize_recipe_data(recipe_data.recipe)
      end
   end
   return craftable_recipes
end

-- Prep the recipe data with any default values
function JobInfoController:_initialize_recipe_data(recipe_data)
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
   recipe_data.product_info = radiant.resources.load_json(recipe_data.produces[1].item)
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
return JobInfoController
