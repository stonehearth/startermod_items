--[[
   The Workshop.lua component implements all the functionality
   associated with the place a crafter goes to do the work a user
   assigns to him. A profession's workplace.txt file (for example,
   carpenter_workbench.json references this blob of code as a component.

   Conceptually, all workshops have an intermediate item--the item the crafter
   is working on right now. If the crafter is not working on an item,
   the current item is nil. If the crafter is working on an item, then
   the current item has a recipe and a status for its progress.
]]

-- All workshops have a ToDo list through which the user instructs the crafter
local RadiantIPoint3 = _radiant.math3d.RadiantIPoint3
local ToDoList = radiant.mods.require('/stonehearth_crafter/lib/todo_list.lua')
local CraftOrder = radiant.mods.require('/stonehearth_crafter/lib/craft_order.lua')

local Workshop = class()

function Workshop:__init(entity, data_blob)
   self._todo_list = ToDoList(data_blob)        -- The list of things we need to work on
   self._backing_obj = backing_obj
   self._entity = entity               -- The entity associated with this component
   self._curr_order = nil              -- The order currently being worked on. Nil until we get an order from the todo list
   self._intermediate_item = nil       -- The item currently being worked on. Nil until we actually start crafting
                                       -- TODO: revise all three of these to use entity-container
   self._bench_outputs = {}            -- An array of finished products on the bench, to be added to the outbox. Nil if nothing.
   self._outbox = nil                  -- The outbox entity
   self._saw = nil                     -- The saw for the bench, available when there is no craftsman
end

function Workshop:extend(json)
end

function Workshop:__tojson()
   local json = {
      crafter = self._crafter,
      order_list = self._todo_list,
      is_paused = self:is_paused()
   }
   return radiant.json.encode(json)
end

--[[UI Interaction Functions
   Functions called by the UI through posts
]]

--[[
   Creates an order and sticks it into the todo list.
   Order_data has to contain
   recipe_url: url to the recipe
   TODO: types of ingredients
   condition_amount: an integer,
         OR
   condition_inventory_below:integer to mantain at
   Returns: true if successful add, false otherwise
--]]
function Workshop:add_order(player_object, order_data)
   local condition = {}
   if order_data.condition_amount then
      condition.amount = tonumber(order_data.condition_amount)
   else
      condition.inventory_below = tonumber(order_data.condition_inventory_below)
   end
   local order = CraftOrder(radiant.resources.load_json(order_data.recipe_url), true,  condition, self)
   self._todo_list:add_order(order)
   --TODO: if something fails and we know it, send error("string explaining the error") anywhere
   --to be caught by the result variable
end

--[[
   Given a recipe and order options, return a picture and description for
   the object with the options selected. Order_options should contain:
   recipe_url: url to the recipe
   TODO: specifically which kind of ingredient per ingredient type
   TODO: Define the template/mechanism for options
   Returns: true if successful, false with message otherwise,
   and url and description of image
]]
function Workshop:resolve_order_options(player_object, order_options)
   local recipe = radiant.resources.load_json(order_options.recipe_url)
   return {portrait = recipe.card_art, desc = recipe.description, flavor = recipe.flavor}
end

--[[
   Causes the crafter to pause in his progress through the list
   at the next convenient moment.
   should_pause: true if we should pause, false if we should unpause.
]]
function Workshop:toggle_pause(player_object, data)
   self._todo_list:togglePause()
end

function Workshop:is_paused()
   return self._todo_list:is_paused()
end

--[[
   Tell the todo list to move the order (by id) to the new position
   order_list_data.newPos.
]]
function Workshop:move_order(player_object, order_list_data)
   self._todo_list:change_order_position(order_list_data.newPos, order_list_data.id)
end

--[[
   Delete an order and clean up if it's the current order
]]
function Workshop:delete_order(player_object, target_order)
   self._todo_list:remove_order(target_order.id)
   if  self._curr_order and target_order.id == self._curr_order:get_id() then
      if self._intermediate_item then
         radiant.entities.destroy_entity(self._intermediate_item.entity)
         self._intermediate_item = nil
      end
      self._curr_order = nil
   end
end

--[[
   Public Functions
]]

--[[
   Returns the entity of the work area
   associated with this workshop
]]
function Workshop:get_entity()
   return self._entity
end


--[[
   Returns the crafter associated with this workshop
]]
function Workshop:get_crafter()
   return self._crafter
end

function Workshop:get_curr_order()
   return self._curr_order
end

--[[
   This is the outbox component (not the entity)
]]
function Workshop:set_outbox(outbox)
   self._outbox = outbox
end

function Workshop:get_outbox()
   return self._outbox
end

--[[
   Associate a crafter entity with this bench.
]]
function Workshop:set_crafter(crafter)
   local current = self:get_crafter()
   if not crafter or not current or current:get_id() ~= crafter:get_id() then
      self._crafter = crafter
   end
   self._backing_obj:mark_changed()
end

--[[
   When there isn't a crafter yet, associate a saw
   If the saw is nil, and a saw currently exists, remove the saw from the bench
]]

function Workshop:set_saw_entity(saw_entity)
   self._saw = saw_entity
   local saw_loc = radiant.entities.get_location_aligned(self._entity)
   --TODO: how do we get the saw higher in the world?
   radiant.terrain.place_entity(saw_entity, RadiantIPoint3(saw_loc.x, 3, saw_loc.z + 3))
end

function Workshop:get_saw_entity()
   return self._saw
end

--[[
   Get the crafter component of the crafter entity associated with
   this workshop
]]
function Workshop:_get_crafter_component()
   local crafter_entity = self:get_crafter()
   if crafter_entity then
      return crafter_entity:get_component('stonehearth_crafter:crafter')
   end
end

--[[
   returns: true if there is an output on the bench, false
   otherwise
]]
function Workshop:has_bench_outputs()
   return #self._bench_outputs > 0
end

--[[
   Pops an entity off the bench and returns it
   returns: an entity from the bench, or nil if there
   are no entities left. (No return is a return of nil.)
]]
function Workshop:pop_bench_output()
   if #self._bench_outputs > 0 then
      return table.remove(self._bench_outputs)
   end
end

--[[
   Get the next thing off the top of the todo list.
   Also sets the current order for the workshop.
   If the current order already exists (because we were interrupted
   before we could complete it) just keep going with it.
   TODO: What happens if the ingredients are invalidated?
   returns: the next recipe on the todo list (if any, nil if there
            is no next order), and a list of ingredients that still
            need to be collected in order to execute the recipe.
]]
function Workshop:establish_next_craftable_recipe()
   if self._curr_order == nil then
      local order, ingredients = self._todo_list:get_next_task()
      self._curr_order = order
      self._current_ingredients = ingredients
   end
   return self:_get_current_recipe(), self._current_ingredients
end

--[[
   Returns an array of all the items currently on the bench.
]]
function Workshop:get_items_on_bench()
   local children = self:get_entity():add_component('entity_container'):get_children()

   local items = {}
   for _, item in children:items() do
      table.insert(items, item)
   end
   return items
end

--[[
   The intermediate item only exists once all the items are on
   the workbench and we are currently crafting. We can use it
   to figure out if we are in the process of building someting.
   Returns: True if we've collected all the materials and are
            in the process of making something. False otherwise.
]]
function Workshop:is_currently_crafting()
   return self._intermediate_item ~= nil
end

--[[
   This function does one unit's worth of progress on the current
   recipe. If we haven't started working on the recipe yet,
   it creates the intermediate item. If we're done, then it
   destroys it and creates the outputs.
   ai: the ai associated with an action
   returns: true if there is more work to be done, and false if
            there is no work to be done
]]
function Workshop:work_on_curr_recipe(ai)
   if self._curr_order == nil then
      return true
   end
   if not self._intermediate_item then
      self:_create_intemediate_item()
   end
   local work_units = self:_get_current_recipe().work_units
   if self._intermediate_item.progress < work_units then
      self:_get_crafter_component():perform_work_effect(ai)
      if self._intermediate_item then
         self._intermediate_item.progress = self._intermediate_item.progress + 1
      end
      return true
   else
      self:_crafting_complete()
      return false
   end
end

--[[
   Create the item that represents the crafter's progress
   on the current item in the workshop. An intermediate item
   tracks its progress and its entity.
   To create the intermediate item, we must first remove all
   the ingredients from the bench. We assume all the ingredients
   are present. If crafting is interrupted,
   Returns: the intermediate item
]]
function Workshop:_create_intemediate_item()
   --verify that the ingredients for the curr recipe are on the bench
   assert(self:_verify_curr_recipe(), "Can't find all ingredients in recipe")

   -- now we can destroy all the items on the bench and create an
   -- intermediate item.
   local items = self:get_items_on_bench()
   for i, item in ipairs(items) do
      radiant.entities.destroy_entity(item)
   end

   --Create intermediate item (with progress) and place its entity in the world
   local intermediate_item = radiant.entities.create_entity(self:_get_crafter_component():get_intermediate_item())
   --TODO: how to get intermediat item onto the top of the world?
   radiant.entities.add_child(self._entity, intermediate_item, RadiantIPoint3(0, 1, 0))
   self._intermediate_item = {
      progress = 0,
      entity = intermediate_item
   }
   return self._intermediate_item
end

--[[
   Compare the ingredients in the recipe to the ingredients in the bench
   returns: True if all ingredients are on bench. False otherwise.
]]
function Workshop:_verify_curr_recipe()
   if self._current_ingredients then
      -- verify that all the items in the current ingredients are on the
      -- bench, and nothing else!
      local ec = self._entity:get_component('entity_container'):get_children()
      for _, ingredient in pairs(self._current_ingredients) do
         if not ec:get(ingredient.item:get_id()) then
            return false
         end
      end
      return #self._current_ingredients <= ec:size()
   end
end

--[[
   Reset all the things that hold the intermediate state
   and place the workshop outputs into the world.
]]
function Workshop:_crafting_complete()
   radiant.entities.destroy_entity(self._intermediate_item.entity)
   self._intermediate_item = nil

   self:_produce_outputs()

   self._todo_list:chunk_complete(self._curr_order)
   self._curr_order = nil

end

--[[
   Produces all the things in the recipe, puts them in the world.
   TODO: handle unwanted outputs, like toxic waste
]]
function Workshop:_produce_outputs()
   local recipe = self:_get_current_recipe()
   local outputs = recipe.produces
   self._bench_outputs = {}
   for i, product in radiant.resources.pairs(outputs) do
      local result = radiant.entities.create_entity(product.item)
      --TODO: use entity container to put items on the bench
      radiant.terrain.place_entity(result, RadiantIPoint3(-12, -5, -12))
      table.insert(self._bench_outputs, result)
   end
end

--[[
   Helper function, returns the recipe in the current order.
]]
function Workshop:_get_current_recipe()
   if(self._curr_order) then
      return self._curr_order:get_recipe()
   end
end


--[[
   This function is only available as a courtesy to the
   ui. Other gameplay modules shouldn't actually be able
   to access the todo list.
]]
function Workshop:ui_get_todo_list()
   return self._todo_list
end

return Workshop
