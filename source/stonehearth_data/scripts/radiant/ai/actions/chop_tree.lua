local ChopTree = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.chop_tree', ChopTree)

function ChopTree:run(ai, entity, tree)
   check:is_entity(entity)
   if tree:is_valid() then
      check:is_entity(tree)
    
      om:turn_to_face(entity, om:get_world_grid_location(tree))  
      ai:execute('radiant.actions.perform', 'chop')
      md:send_msg(tree, "radiant.resource_node.spawn_resource", om:get_grid_location(entity))
   end
end
