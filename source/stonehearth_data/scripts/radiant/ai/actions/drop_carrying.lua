local DropCarrying = class()

local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.drop_carrying', DropCarrying)

function DropCarrying:run(ai, entity, location)
   check:is_entity(entity)
   check:is_a(location, Point3)
    
   if om:get_carrying(entity) then
      om:turn_to_face(entity, location)
      ai:execute('radiant.actions.perform', 'carry_putdown')
      om:drop_carrying(entity, location)
   end
end
