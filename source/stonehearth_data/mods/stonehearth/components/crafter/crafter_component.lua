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

   -- xxx: show_workshop.js should just ask the inventory itself!! - tony
   local inventory = stonehearth.inventory:get_inventory(self._entity)
   self:_determine_maintain()

   self._listeners = {}
   local function add_listener_to_inventory(evt, fn)
      local listener = radiant.events.listen(inventory, evt, self, fn)
      table.insert(self._listeners, listener)
   end
   add_listener_to_inventory('stonehearth:inventory:stockpile_added',   self._determine_maintain)
   add_listener_to_inventory('stonehearth:inventory:stockpile_removed', self._determine_maintain)
   add_listener_to_inventory('stonehearth:inventory:storage_added',     self._determine_maintain)
   add_listener_to_inventory('stonehearth:inventory:storage_removed',   self._determine_maintain)
end

function CrafterComponent:destroy()
   if self._orchestrator then
      self._orchestrator:destroy()
      self._orchestrator = nil
   end
   if self._sv.workshop and self._sv.workshop:get_entity() and self._sv.workshop:get_entity():is_valid() then
      self._sv.workshop:set_crafter(nil)
   end
   local inventory = stonehearth.inventory:get_inventory(self._entity)

   for _, l in pairs(self._listeners) do
      l:destroy()
   end
   self._listeners = {}
end

-- True if there are stockpiles, false otherwise. 
-- TODO: consider elaborating to whether the stockpiles can contain stuff crafted by this crafter
function CrafterComponent:_determine_maintain()
   local inventory = stonehearth.inventory:get_inventory(self._entity)
   local stockpiles = inventory:get_all_stockpiles()
   local public_storage = inventory:get_all_public_storage()

   self._sv.should_maintain = (not radiant.empty(stockpiles)) or (not radiant.empty(public_storage))
   self.__saved_variables:mark_changed()
end

function CrafterComponent:should_maintain()
   return self._sv.should_maintain
end

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
   self.__saved_variables:mark_changed()
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
