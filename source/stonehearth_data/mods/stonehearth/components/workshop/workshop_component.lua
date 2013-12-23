--[[
   The WorkshopComponent.lua component implements all the functionality
   associated with the place a crafter goes to do the work a user
   assigns to him. A profession's workplace.txt file (for example,
   carpenter_workbench.json references this blob of code as a component.

]]

-- All WorkshopComponents have a ToDo list through which the user instructs the crafter
local Point3 = _radiant.csg.Point3
local CraftOrder = require 'components.workshop.craft_order'
local CraftOrderList = require 'components.workshop.craft_order_list'
local Analytics = require 'services.analytics.analytics_service'

local WorkshopComponent = class()

function WorkshopComponent:__init(entity, data_binding)
   self._todo_list = CraftOrderList(data_binding)  -- The list of things we need to work on
   self._entity = entity                 -- The entity associated with this component
   self._curr_order = nil                -- The order currently being worked on. Nil until we get an order from the todo list
   self._curernt_item_progress = nil
                                         -- TODO: revise all three of these to use entity-container
   self._bench_outputs = {}              -- An array of finished products on the bench, to be added to the outbox. Nil if nothing.
   self._outbox_entity = nil
   self._outbox_component = nil          -- The outbox
   self._promotion_talisman_entity = nil -- The talisman for the bench, available when there is no craftsman
   self._promotion_talisman_offset = {0, 3, 0}  -- Default offset for the talisman (on the bench)

   self._data = data_binding:get_data()
   self._data.crafter = nil
   self._data.order_list = self._todo_list

   self._data_binding = data_binding
   self._data_binding:mark_changed()
end

function WorkshopComponent:extend(json)
   if json then
      if json.promotion_talisman.entity then
         self._promotion_talisman_entity_uri = json.promotion_talisman.entity
      end
      if json.promotion_talisman.offset then
         self._promotion_talisman_offset = json.promotion_talisman.offset
      end

      if json.skin_class then 
         self._data.skin_class = json.skin_class
      else 
         --xxx populate a default skin
      end
   end
   self._data_binding:mark_changed()
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
function WorkshopComponent:add_order(session, response, recipe, condition)
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
function WorkshopComponent:resolve_order_options(session, response, recipe)
   return {portrait = recipe.portrait, description = recipe.description, flavor = recipe.flavor}
end

--[[
   Causes the crafter to pause in his progress through the list
   at the next convenient moment.
   should_pause: true if we should pause, false if we should unpause.
]]
function WorkshopComponent:toggle_pause(sesion, response)
   self._data.is_paused = not self._data.is_paused
   self._data_binding:mark_changed()
end

function WorkshopComponent:is_paused()
   return self._data.is_paused
end

--[[
   Tell the todo list to move the order (by id) to the new position
   order_list_data.newPos.
]]
function WorkshopComponent:move_order(session, response, id, newPos)
   self._todo_list:change_order_position(newPos, id)
   return true
end

--[[
   Delete an order and clean up if it's the current order
]]
function WorkshopComponent:delete_order(session, response, id)
   self._todo_list:remove_order(id)
   if self._curr_order and id == self._curr_order:get_id() then
      self._current_item_status = nil
      self._curr_order = nil
   end
   return true
end

--[[
   Public Functions
]]

--[[
   Returns the entity of the work area
   associated with this WorkshopComponent
]]
function WorkshopComponent:get_entity()
   return self._entity
end


--[[
   Returns the crafter associated with this WorkshopComponent
]]
function WorkshopComponent:get_crafter()
   return self._data.crafter
end

function WorkshopComponent:get_curr_order()
   return self._curr_order
end

function WorkshopComponent:get_talisman() 
   return self._promotion_talisman_entity
end

--[[
   Associate a crafter entity with this bench.
]]
function WorkshopComponent:set_crafter(crafter)
   local current = self:get_crafter()
   if not crafter or not current or current:get_id() ~= crafter:get_id() then
      self._data.crafter = crafter
      self._data_binding:mark_changed()

      local commandComponent = self._entity:get_component('stonehearth:commands')
      if crafter then
         commandComponent:enable_command('show_workshop', true)
      else
         commandComponent:enable_command('show_workshop', false);
      end

      local show_workshop_command = crafter:add_component('stonehearth:commands')
                                           :add_command('/stonehearth/data/commands/show_workshop_from_crafter')


      show_workshop_command.event_data = {
         workshop = self._entity
      }

      self._entity:add_component('stonehearth:commands'):remove_command('promote_to_profession')

      -- xxx, localize                                          
      local crafter_name = radiant.entities.get_name(crafter)
      radiant.entities.set_description(self._entity, 'owned by ' .. crafter_name)

   end
end

function WorkshopComponent:init_from_scratch()
   return self:init_promotion_talisman()
end

--[[
   Associate a promotion talisman with the workbench. A worker will use the talisman
   to promote himself to a crafter
   If the talisman is nil, and a talisman currently exists, remove the talisman from the bench
]]
function WorkshopComponent:init_promotion_talisman()
   local offset = self._promotion_talisman_offset

   self._promotion_talisman_entity = radiant.entities.create_entity(self._promotion_talisman_entity_uri)
   self._entity:add_component('entity_container'):add_child(self._promotion_talisman_entity)
   self._promotion_talisman_entity:add_component('mob'):set_location_grid_aligned(Point3(offset[1], offset[2], offset[3]))

   self._promotion_talisman_entity:get_component('stonehearth:promotion_talisman'):set_workshop(self)

   return self._promotion_talisman_entity
end

function WorkshopComponent:get_promotion_talisman_entity_uri()
   return self._promotion_talisman_entity_uri
end

--- Add an existing outbox to the workshop
--  Note: outbox should have a faction already
function WorkshopComponent:associate_outbox(outbox_entity)
   self._outbox_entity = outbox_entity
   self._outbox_component = self._outbox_entity:get_component('stonehearth:stockpile')
   return self._outbox_entity
end

function WorkshopComponent:get_outbox_entity()
   return self._outbox_entity
end

--[[
   Get the crafter component of the crafter entity associated with
   this WorkshopComponent
]]
function WorkshopComponent:_get_crafter_component()
   local crafter_entity = self:get_crafter()
   if crafter_entity then
      return crafter_entity:get_component('stonehearth:crafter')
   end
end

--[[
   returns: true if there is an output on the bench, false
   otherwise
]]
function WorkshopComponent:has_bench_outputs()
   return #self._bench_outputs > 0
end

--[[
   Pops an entity off the bench and returns it
   returns: an entity from the bench, or nil if there
   are no entities left. (No return is a return of nil.)
]]
function WorkshopComponent:pop_bench_output()
   if #self._bench_outputs > 0 then
      return table.remove(self._bench_outputs)
   end
end

--[[
   Get the next thing off the top of the todo list.
   Also sets the current order for the WorkshopComponent.
   If the current order already exists (because we were interrupted
   before we could complete it) just keep going with it.
   TODO: What happens if the ingredients are invalidated?
   returns: the next recipe on the todo list (if any, nil if there
            is no next order), and a list of ingredients that still
            need to be collected in order to execute the recipe.
]]
function WorkshopComponent:establish_next_craftable_recipe()
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
function WorkshopComponent:get_items_on_bench()
   local ec = self:get_entity():add_component('entity_container')

   local items = {}
   for _, item in ec:each_child() do
      table.insert(items, item)
   end
   return items
end

--[[
   Returns: True if we've collected all the materials and are
            in the process of making something. False otherwise.
]]
function WorkshopComponent:is_currently_crafting()
   return self._current_status ~= nil
end

--[[
   This function does one unit's worth of progress on the current
   recipe. 
   ai: the ai associated with an action
   returns: true if there is more work to be done, and false if
            there is no work to be done
]]
function WorkshopComponent:work_on_curr_recipe(ai)
   if self._curr_order == nil then
      return true
   end
   if not self._current_status then
      self._current_status = {
         progress = 0
      }
   end
   local work_units = self:_get_current_recipe().work_units
   if self._current_status.progress < work_units then
      self:_get_crafter_component():perform_work_effect(ai)
      if self._current_status then
         self._current_status.progress = self._current_status.progress + 1
      end
      return true
   else
      self:_crafting_complete()
      return false
   end
end

--[[
   Compare the ingredients in the recipe to the ingredients in the bench
   returns: True if all ingredients are on bench. False otherwise.
]]
function WorkshopComponent:_verify_curr_recipe()
   if self._current_ingredients then
      -- verify that all the items in the current ingredients are on the
      -- bench, and nothing else!
      local ec = self._entity:get_component('entity_container')

      for _, ingredient in pairs(self._current_ingredients) do
         if not radiant.entities.has_child_by_id(self._entity, ingredient.item:get_id()) then
            return false
         end
      end
      return #self._current_ingredients <= ec:num_children()
   end
end

--[[
   Reset all the things that hold the intermediate state
   and place the WorkshopComponent outputs into the world.
]]
function WorkshopComponent:_crafting_complete()
   self._current_status = nil  
   self:_produce_outputs()

   self._todo_list:chunk_complete(self._curr_order)
   self._curr_order = nil

end

--[[
   Produces all the things in the recipe, puts them in the world.
TODO: handle unwanted outputs, like toxic waste
]]
function WorkshopComponent:_produce_outputs()
   local recipe = self:_get_current_recipe()
   local outputs = recipe.produces
   self._bench_outputs = {}

   -- destroy all the ingredients on the bench
   local items = self:get_items_on_bench()
   for i, item in ipairs(items) do
      radiant.entities.destroy_entity(item)
   end

   -- create all the recipe products
   for i, product in ipairs(outputs) do
      local result = radiant.entities.create_entity(product.item)

      self._entity:add_component('entity_container'):add_child(result)
      result:add_component('mob'):set_location_grid_aligned(Point3(0, 1, 0))
      table.insert(self._bench_outputs, result)

      Analytics:send_design_event('game:craft', self._entity, result)
   end
end

--[[
   Helper function, returns the recipe in the current order.
]]
function WorkshopComponent:_get_current_recipe()
   if self._curr_order then
      return self._curr_order:get_recipe()
   end
end


--[[
   This function is only available as a courtesy to the
   ui. Other gameplay modules shouldn't actually be able
   to access the todo list.
]]
function WorkshopComponent:ui_get_todo_list()
   return self._todo_list
end

return WorkshopComponent
