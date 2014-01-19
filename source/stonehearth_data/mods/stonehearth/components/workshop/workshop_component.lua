--[[
   The WorkshopComponent.lua component implements all the functionality
   associated with the place a crafter goes to do the work a user
   assigns to him. A profession's workplace.txt file (for example,
   carpenter_workbench.json references this blob of code as a component.

]]

-- All WorkshopComponents have a ToDo list through which the user instructs the crafter
local Point3 = _radiant.csg.Point3
local CraftOrderList = require 'components.workshop.craft_order_list'

local WorkshopComponent = class()

function WorkshopComponent:__init(entity, data_binding)
   self._craft_order_list = CraftOrderList(data_binding)  -- The list of things we need to work on
   self._entity = entity
   self._bench_outputs = {}              -- An array of finished products on the bench, to be added to the outbox. Nil if nothing.
   self._outbox_entity = nil
   self._promotion_talisman_entity = nil -- The talisman for the bench, available when there is no craftsman
   self._promotion_talisman_offset = Point3(0, 3, 0)  -- Default offset for the talisman (on the bench)
self._data = data_binding:get_data()
   self._data.crafter = nil
   self._data.order_list = self._craft_order_list

   self._data_binding = data_binding
   self._data_binding:mark_changed()
   
end

function WorkshopComponent:extend(json)
   if json then
      self._construction_ingredients = json.ingredients
      self._build_sound_effect = json.build_sound_effect

      if json.promotion_talisman.entity then
         self._promotion_talisman_entity_uri = json.promotion_talisman.entity
      end
      if json.promotion_talisman.offset then
         local o = json.promotion_talisman.offset
         self._promotion_talisman_offset = Point3(o.x, o.y, o.z)
      end

      if json.skin_class then 
         self._data.skin_class = json.skin_class
      else 
         --xxx populate a default skin
      end
      if json.profession_name then
         --TODO: specific crafter workshops should get their commands from a mixin,
         --but the mixins never load before the extend functions.
         --Make a more solid guarantee system of getting commands before this point.
         self._profession_name = json.profession_name
         local command_component = self._entity:get_component('stonehearth:commands')
         if command_component then
            command_component:modify_command('promote_to_profession', function(command) 
                  command.event_data.promotion_name = json.profession_name     
               end)
         end
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
   self._craft_order_list:remove_order(id)
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

      self:_create_scheduler(crafter)
   end
end

-- Associate a promotion talisman with the workbench. A worker will use the talisman
-- to promote himself to a crafter
-- If the talisman is nil, and a talisman currently exists, remove the talisman from the bench
function WorkshopComponent:_create_promotion_talisman(faction)
   local offset = self._promotion_talisman_offset

   self._promotion_talisman_entity = radiant.entities.create_entity(self._promotion_talisman_entity_uri)
   self._entity:add_component('entity_container'):add_child(self._promotion_talisman_entity)
   self._promotion_talisman_entity:add_component('mob'):set_location_grid_aligned(offset)

   local promotion_talisman_component = self._promotion_talisman_entity:get_component('stonehearth:promotion_talisman')
   if promotion_talisman_component then
      promotion_talisman_component:set_workshop(self)
      promotion_talisman_component:set_profession_name(self._profession_name)
   end

   self._promotion_talisman_entity:get_component('unit_info'):set_faction(faction)

   return self._promotion_talisman_entity
end

function WorkshopComponent:get_outbox_entity()
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

--[[
   This function is only available as a courtesy to the
   ui. Other gameplay modules shouldn't actually be able
   to access the todo list.
]]
function WorkshopComponent:ui_get_craft_order_list()
   return self._craft_order_list
end

function WorkshopComponent:finish_construction(faction, outbox)
   self._entity:add_component('unit_info'):set_faction(faction)
   
   -- Place the promotion talisman on the workbench, if there is one
   self:_create_promotion_talisman(faction)
   self._outbox_entity = outbox
end

function WorkshopComponent:_create_scheduler(crafter)
   self._scheduler = stonehearth.tasks:create_scheduler()
                        :set_activity('stonehearth:craft_item')
                        :join(crafter)
end

return WorkshopComponent
