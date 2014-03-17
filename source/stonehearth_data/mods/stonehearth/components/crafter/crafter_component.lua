--[[
   Crafter.lua implements all functionality associated with a
   crafter citizen. 
]]

local CrafterComponent = class()
local CreateWorkshop = require 'services.town.orchestrators.create_workshop_orchestrator'

function CrafterComponent:initialize(entity, json)
   self._entity = entity
   self._work_effect = json.work_effect
   if json.recipe_list then
      self._recipe_list = radiant.resources.load_json(json.recipe_list)
      self._craftable_recipes = self._recipe_list.craftable_recipes
   else
      self._recipe_list = {}
      self._craftable_recipes = {}
   end
   
   self.__saved_variables = radiant.create_datastore({
      craftable_recipes = self._craftable_recipes
   })
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
   outbox_entity:get_component('unit_info'):set_faction(faction)

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
   if workshop_component ~= self._workshop then
      self._workshop = workshop_component   
      radiant.events.trigger(self._entity, 'stonehearth:crafter:workshop_changed', {
            entity = self._entity,
            workshop = self._workshop,
         })
   end
end

function CrafterComponent:get_workshop()
   return self._workshop
end

return CrafterComponent
