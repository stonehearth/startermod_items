--[[
   Crafter.lua implements all functionality associated with a
   crafter citizen. 
]]

local CrafterComponent = class()
local CreateWorkshop = require 'services.server.town.orchestrators.create_workshop_orchestrator'

function CrafterComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   self._work_effect = json.work_effect
   if not self._sv._initialized then
      self._sv._initialized = true

      local craftable_recipes = {}
      if json.recipe_list then
         local recipe_list = radiant.resources.load_json(json.recipe_list)
         craftable_recipes = recipe_list.craftable_recipes or {}
      end

      self._sv.craftable_recipes = craftable_recipes
      self._sv.active = false
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

function CrafterComponent:get_work_effect()
   return self._work_effect
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

   -- create a task group for the workshop.  we'll use this both to build it and
   -- to feed the crafter orders when it's finally created
   local town = stonehearth.town:get_town(self._entity)
   self._orchestrator = town:create_orchestrator(CreateWorkshop, {
      crafter = self._entity,
      ghost_workshop = ghost_workshop,
      outbox_entity = outbox_entity,
   })
end

function CrafterComponent:set_workshop(workshop_component)
   if workshop_component ~= self._sv.workshop then
      self._sv.workshop = workshop_component
      self.__saved_variables:mark_changed()

      radiant.events.trigger(self._entity, 'stonehearth:crafter:workshop_changed', {
            entity = self._entity,
            workshop = self._sv.workshop,
         })
   end
end

function CrafterComponent:get_workshop()
   return self._sv.workshop
end

return CrafterComponent
