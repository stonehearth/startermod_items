-- xxx xxx xxx : can we just get rid of this?  what's the difference between sh and om?
-- i guess sh is higher level, but so what?  why aren't these free functions (or factored
-- into om)

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local check = require 'radiant.core.check'

-- xxx: break this up into the init-time and the run-time reactor?
-- perhaps by making the run-time part a singleton??

local Stonehearth = class()

function Stonehearth:start_new_game()
   self._promises = {}
   self._worker_scheduler = md:create_msg_handler('radiant.msg_handlers.worker_scheduler', 'civ')
   
   local root = om:get_root_entity()
   local container = om:get_component(root, 'entity_container')

   local promise = container:trace_children()
                     :added(function(id, entity) self:_on_add_entity(id, entity) end)
                     :removed(function(id) self:_on_remove_entity(id) end)
   table.insert(self._promises, promise)

   -- ug... the build_orders object needs to be in lua, not c++... or at least we need
   -- multiple build_orders for every script that wants to build something!
   promise = om:get_build_orders():trace_in_progress()
                     :added(function(build_order) self:_add_build_order(build_order) end)
                     :removed(function(build_order) self:_remove_build_order(build_order) end)
   table.insert(self._promises, promise)
   
end

function Stonehearth:get_worker_scheduler()
   return self._worker_scheduler
end

function Stonehearth:harvest(entity)
   md:send_msg(self._worker_scheduler, "radiant.resource_node.harvest", entity)
end

Stonehearth.CitizenNames = {
   'Mer Burlyhands',
   'Illowyn Farstrider',
   'Samson Moonstoke',
   'Godric Greenskin',
   'Olaf Oakshield',
}

local first_citizen = false
function Stonehearth:create_citizen(location, profession)
   profession = profession and profession or 'worker'
   check:is_a(location, RadiantIPoint3)
   check:is_string(profession)

   local entity 
   if first_citizen then
      entity = om:create_entity('module://stonehearth/mobs/civ_black')
      first_citizen = false
   else 
      entity = om:create_entity('module://stonehearth/mobs/civ')
   end
   om:place_on_terrain(entity, location)

   om:set_display_name(entity, Stonehearth.CitizenNames[math.random(1, #Stonehearth.CitizenNames)])
   om:get_component(entity, 'mob'):turn_to(math.random(0, 359))
   
   -- xxx: drive some of this from json
   om:add_component(entity, 'inventory'):set_capacity(8)
   om:add_component(entity, 'carry_block')
   if profession == 'worker' then
      md:add_msg_handler(entity, 'radiant.professions.worker', self._worker_scheduler)
   else
      md:add_msg_handler(entity, 'radiant.professions.'..profession)
   end

   -- xxx: factor md:add_msg_handler(entity, 'radiant.needs.idle')  
   --[[
   md:add_msg_handler(entity, 'radiant.behaviors.run_user_queue')
   md:add_msg_handler(entity, 'radiant.behaviors.run_automation_queue')
   md:add_msg_handler(entity, 'radiant.needs.panic')
   md:add_msg_handler(entity, 'radiant.behaviors.panic')
   
   -- run away from scary things our sight radius
   md:add_msg_handler(entity, 'radiant.msg_handlers.panic_check')
   ]] 
   
   return entity
end

function Stonehearth:_on_add_entity(id, entity)
   if entity then
      if om:has_component(entity, 'item') then
         self._worker_scheduler:add_item(entity)
      end
      if om:has_component(entity, 'stockpile_designation') and om:get_faction(entity) == 'civ' then
         self._worker_scheduler:add_stockpile(entity)
      end
   end
end

function Stonehearth:_on_remove_entity(id)
   --self._worker_scheduler:remove_item(id)
   --self._worker_scheduler:remove_stockpile(id)
end

function Stonehearth:_add_build_order(build_order)
   self._worker_scheduler:add_build_order(build_order)
end

function Stonehearth:_remove_build_order(build_order)
   self._worker_scheduler:remove_build_order(build_order)
end

return Stonehearth()
