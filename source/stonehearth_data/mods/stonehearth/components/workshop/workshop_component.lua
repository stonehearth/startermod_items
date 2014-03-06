--[[
   The WorkshopComponent.lua component implements all the functionality
   associated with the place a crafter goes to do the work a user
   assigns to him. A profession's workplace.txt file (for example,
   carpenter_workbench.json references this blob of code as a component.

]]

local Point3 = _radiant.csg.Point3
local CraftOrderList = require 'components.workshop.craft_order_list'

local WorkshopComponent = class()

function WorkshopComponent:__create(entity, json)
   self._entity = entity
   self._bench_outputs = {}              -- An array of finished products on the bench, to be added to the outbox. Nil if nothing.
   self._outbox_entity = nil

   self._craft_order_list = CraftOrderList()
   self.__savestate = radiant.create_datastore({
         order_list = self._craft_order_list,
         skin_class = json.skin_class or 'default'
      })
   self.__savestate:set_controller(self)
   self._construction_ingredients = json.ingredients
   self._build_sound_effect = json.build_sound_effect
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
   self._craft_order_list:add_order(recipe, condition, radiant.entities.get_faction(self._entity))
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
   self._craft_order_list:toggle_pause()
   return true
end

--[[
   Tell the todo list to move the order (by id) to the new position
   order_list_data.newPos.
]]
function WorkshopComponent:move_order(session, response, id, newPos)
   self._craft_order_list:change_order_position(newPos, id)
   return true
end

--[[
   Delete an order and clean up if it's the current order
]]
function WorkshopComponent:delete_order(session, response, id)
   self._craft_order_list:remove_order_id(id)
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
   return self._crafter
end

--[[
   Associate a crafter entity with this bench.
]]
function WorkshopComponent:set_crafter(crafter)
   local current = self:get_crafter()
   if not crafter or not current or current:get_id() ~= crafter:get_id() then
      self._crafter = crafter
      self.__savestate:get_data().crafter = crafter
      self.__savestate:mark_changed()

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

      crafter:add_component('stonehearth:commands'):remove_command('build_workshop')

      -- xxx, localize                                          
      local crafter_name = radiant.entities.get_name(crafter)
      radiant.entities.set_description(self._entity, 'owned by ' .. crafter_name)
   end
end

function WorkshopComponent:get_outbox()
   return self._outbox_entity
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

function WorkshopComponent:get_craft_order_list()
   return self._craft_order_list
end

function WorkshopComponent:finish_construction(faction, outbox_entity)
   self._entity:add_component('unit_info'):set_faction(faction)
   self._outbox_entity = outbox_entity
end

return WorkshopComponent
