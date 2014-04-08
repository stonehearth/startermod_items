--[[
   The WorkshopComponent.lua component implements all the functionality
   associated with the place a crafter goes to do the work a user
   assigns to him. A profession's workplace.txt file (for example,
   carpenter_workbench.json references this blob of code as a component.

]]

local Point3 = _radiant.csg.Point3
local WorkAtWorkshop = require 'services.server.town.orchestrators.work_at_workshop_orchestrator'
local CraftOrderList = require 'components.workshop.craft_order_list'

local WorkshopComponent = class()

function WorkshopComponent:initialize(entity, json)
   self._entity = entity

   self.__saved_variables:set_controller(self)
   self._sv = self.__saved_variables:get_data()

   local orderlist_datastore
   if not self._sv.order_list then
      self._sv.outbox_entity = nil
      self._sv.skin_class = json.skin_class or 'default'
      orderlist_datastore = radiant.create_datastore()
   else
      orderlist_datastore = self._sv.order_list
   end
   self._sv.order_list = CraftOrderList()
   self._sv.order_list.__saved_variables = orderlist_datastore
   self._sv.order_list:initialize()

   self._construction_ingredients = json.ingredients
   self._build_sound_effect = json.build_sound_effect

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         if self._sv.crafter then
            self:_create_workshop_orchestrator()
         end
      end)
end

function WorkshopComponent:destroy()
   if self._orchestrator then
      self._orchestrator:destroy()
      self._orchestrator = nil
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
function WorkshopComponent:add_order(session, response, recipe, condition)
   self._sv.order_list:add_order(recipe, condition, session.player_id)
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
   self._sv.order_list:toggle_pause()
   return true
end

--[[
   Tell the todo list to move the order (by id) to the new position
   order_list_data.newPos.
]]
function WorkshopComponent:move_order(session, response, id, newPos)
   self._sv.order_list:change_order_position(newPos, id)
   return true
end

--[[
   Delete an order and clean up if it's the current order
]]
function WorkshopComponent:delete_order(session, response, id)
   self._sv.order_list:remove_order_id(id)
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
   return self._sv.crafter
end

--[[
   Associate a crafter entity with this bench.
]]
function WorkshopComponent:set_crafter(crafter)
   local current = self:get_crafter()
   if not crafter or not current or current:get_id() ~= crafter:get_id() then
      self._sv.crafter = crafter
      self.__saved_variables:mark_changed()

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

      self:_create_workshop_orchestrator()
   end
end

function WorkshopComponent:_create_workshop_orchestrator()
   local town = stonehearth.town:get_town(self._entity)
   self._orchestrator = town:create_orchestrator(WorkAtWorkshop, {
         workshop = self._entity,
         crafter = self._sv.crafter,
         craft_order_list = self._sv.order_list,
      })
end

function WorkshopComponent:get_outbox()
   return self._sv.outbox_entity
end

function WorkshopComponent:finish_construction(outbox_entity)
   self._sv.outbox_entity = outbox_entity
   self.__saved_variables:mark_changed()
end

return WorkshopComponent
