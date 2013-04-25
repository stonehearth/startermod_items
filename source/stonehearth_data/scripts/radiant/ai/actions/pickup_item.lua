local PickupItem = class()

local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.pickup_item', PickupItem)

function PickupItem:run(ai, entity, item)
   check:is_entity(item)

   local carry_block = om:get_component(entity, 'carry_block')

   -- TODO should this raise an exception if the entity isn't
   -- carrying anything?
 
   -- if we're not close enough to the location, we should
   -- travel there!
   if not carry_block:is_carrying() then
      local location = om:get_world_grid_location(item)
      if om:is_adjacent_to(entity, location) then
         log:info('picking up item at %s', tostring(location))
         om:turn_to_face(entity, location)
         om:pickup_item(entity, item)
         ai:execute('radiant.actions.perform', 'carry_light_pickup')
      end
   end
end
