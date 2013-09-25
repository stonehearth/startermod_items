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
local Point3 = _radiant.csg.Point3
local ToDoList = require 'lib.todo_list'
local CraftOrder = require 'lib.craft_order'

local Workshop = class()

function Workshop:__init(entity, data_binding)
   self._todo_list = ToDoList(data_binding)  -- The list of things we need to work on
   self._entity = entity                 -- The entity associated with this component
   self._curr_order = nil                -- The order currently being worked on. Nil until we get an order from the todo list
   self._intermediate_item = nil         -- The item currently being worked on. Nil until we actually start crafting
                                         -- TODO: revise all three of these to use entity-container
   self._bench_outputs = {}              -- An array of finished products on the bench, to be added to the outbox. Nil if nothing.
   self._outbox_entity = nil
   self._outbox_component = nil          -- The outbox
   self._promotion_talisman_entity = nil -- The talisman for the bench, available when there is no craftsman
   self._promotion_talisman_offset = {0, 3, 0}  -- Default offset for the talisman (on the bench)
   self._outbox_offset = {2, 0, 0}              -- Default offset for the outbox
   self._outbox_size = {3, 3}                   -- Default size for the outbox

   self._data = data_binding:get_data()
   self._data.crafter = nil
   self._data.order_list = self._todo_list

   self._data_binding = data_binding
   self._data_binding:mark_changed()

end

function Workshop:extend(json)
   if json then
      if json.promotion_talisman.entity then
         self._promotion_talisman_entity_uri = json.promotion_talisman.entity
      end
      if json.promotion_talisman.offset then
         self._promotion_talisman_offset = json.promotion_talisman.offset
      end
      if json.outbox_settings.offset then
         self._outbox_offset = json.outbox_settings.offset
      end
      if json.outbox_settings.size then
         self._outbox_size = json.outbox_settings.size
      end
   end
end

--[[UI Interaction Functions
   Functions called by the UI through posts
]]

--[[
   Creates an order and sticks it into the todo list.
   Order_data has to contain
   recipe_url: url to the recipe
   TODO: types of ingredients
   amount: an integer,
         OR
   inventory_below:integer to mantain at
   Returns: true if successful add, false otherwise
--]]
function Workshop:add_order(session, response, recipe, condition)
   local order = CraftOrder(recipe, true,  condition, self)
   self._todo_list:add_order(order)
   --TODO: if something fails and we know it, send error("string explaining the error") anywhere
   --to be caught by the result variable
   return true
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
function Workshop:resolve_order_options(session, response, recipe)
   return {portrait = recipe.portrait, description = recipe.description, flavor = recipe.flavor}
end

--[[
   Causes the crafter to pause in his progress through the list
   at the next convenient moment.
   should_pause: true if we should pause, false if we should unpause.
]]
function Workshop:toggle_pause(sesion, response)
   self._data.is_paused = not self._data.is_paused
   self._data_binding:mark_changed()
end

function Workshop:is_paused()
   return self._data.is_paused
end

--[[
   Tell the todo list to move the order (by id) to the new position
   order_list_data.newPos.
]]
function Workshop:move_order(session, response, id, newPos)
   self._todo_list:change_order_position(newPos, id)
   return true
end

--[[
   Delete an order and clean up if it's the current order
]]
function Workshop:delete_order(session, response, id)
   self._todo_list:remove_order(id)
   if self._curr_order and id == self._curr_order:get_id() then
      if self._intermediate_item then
         radiant.entities.destroy_entity(self._intermediate_item.entity)
         self._intermediate_item = nil
      end
      self._curr_order = nil
   end
   return true
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
   return self._data.crafter
end

function Workshop:get_curr_order()
   return self._curr_order
end


--[[
   Associate a crafter entity with this bench.
]]
function Workshop:set_crafter(crafter)
   local current = self:get_crafter()
   if not crafter or not current or current:get_id() ~= crafter:get_id() then
      self._data.crafter = crafter
      self._data_binding:mark_changed()

      local commandComponent = self._entity:get_component('radiant:commands')
      if crafter then
         commandComponent:enable_command('show_craft_ui', true)
      else
         commandComponent:enable_command('show_craft_ui', false);
      end
   end
end

function Workshop:init_from_scratch()
   return self:init_promotion_talisman(), self:init_outbox()
end

--[[
   Associate a promotion talisman with the workbench. A worker will use the talisman
   to promote himself to a crafter
   If the talisman is nil, and a talisman currently exists, remove the talisman from the bench
]]
function Workshop:init_promotion_talisman()
   local offset = self._promotion_talisman_offset

   self._promotion_talisman_entity = radiant.entities.create_entity(self._promotion_talisman_entity_uri)
   self._entity:add_component('entity_container'):add_child(self._promotion_talisman_entity)
   self._promotion_talisman_entity:add_component('mob'):set_location_grid_aligned(Point3(offset[1], offset[2], offset[3]))

   self._promotion_talisman_entity:get_component('stonehearth_classes:talisman_promotion_info'):set_promotion_data({workshop = self})

   return self._promotion_talisman_entity
end

function Workshop:get_promotion_talisman_entity_uri()
   return self._promotion_talisman_entity_uri
end

--[[
   This is the outbox component (not the entity)
   location: place in the world, relative to workbench, to put the outbox
   size: size of outbox, {width,depth}
   returns: outbox_entity
   TODO: Make this a speciatly stockpile, not like other stockpiles!
]]
function Workshop:init_outbox(custom_offset, custom_size)
   self._outbox_entity = radiant.entities.create_entity('stonehearth_inventory.stockpile')
   local bench_loc = radiant.entities.get_location_aligned(self._entity)

   if custom_offset then
      self._outbox_offset = custom_offset
   end
   local offset = self._outbox_offset
   radiant.terrain.place_entity(self._outbox_entity,
      Point3(bench_loc.x + offset[1], bench_loc.y + offset[2], bench_loc.z + offset[3]))
   local outbox_component = self._outbox_entity:get_component('radiant:stockpile')

   if custom_size then
      self._outbox_size = custom_size
   end
   outbox_component:set_size(self._outbox_size)

   self._outbox_component = outbox_component
   return self._outbox_entity
end

function Workshop:get_outbox_entity()
   return self._outbox_entity
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
   radiant.entities.add_child(self._entity, intermediate_item, Point3(0, 1, 0))
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
         if not radiant.entities.has_child_by_id(self._entity, ingredient.item:get_id()) then
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
   for i, product in ipairs(outputs) do
      local result = radiant.entities.create_entity(product.item)

      self._entity:add_component('entity_container'):add_child(result)
      result:add_component('mob'):set_location_grid_aligned(Point3(0, 1, 0))
      table.insert(self._bench_outputs, result)
   end
end

--[[
   Helper function, returns the recipe in the current order.
]]
function Workshop:_get_current_recipe()
   if self._curr_order then
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
