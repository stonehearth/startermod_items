local Teardown = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.tear_down', Teardown)

function Teardown:run(ai, entity, structure, block)
   if not om:can_teardown_structure(entity, structure) then
      ai:abort()
   end

   local origin = om:get_world_grid_location(structure:get_entity())
   local facing = origin + block
   log:info("structure is at %s working on block %s", tostring(origin), tostring(block))
   log:info("entity is at    %s", tostring(om:get_world_grid_location(entity)))
   om:turn_to_face(entity, facing)         
   ai:execute('radiant.actions.perform', 'work')
   
   structure:construct_block(block)
   om:teardown(entity, structure)
end
