local Construct = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.construct', Construct)

function Construct:run(ai, entity, structure, block)

   local carry_block = om:get_component(entity, 'carry_block')
   if structure:get_class_name() == 'Fixture' then
      structure:set_item(om:create_entity(structure:get_kind()))
   else
      if carry_block:is_carrying() then
         local origin = om:get_world_grid_location(structure:get_entity())
         local facing = origin + block
         log:info("structure is at %s working on block %s", tostring(origin), tostring(block))
         log:info("entity is at    %s", tostring(om:get_world_grid_location(entity)))
         om:turn_to_face(entity, facing)         
         ai:execute('radiant.actions.perform', 'work')

         structure:construct_block(block)
         local carrying = carry_block:get_carrying()
         local item = om:get_component(carrying, 'item')
         local stacks = item:get_stacks()
         if stacks > 1 then
            log:info('dropping carry block stack count to %d in construct.', stacks - 1)
            item:set_stacks(stacks - 1)         
         else
            log:info('clearing carry block in construct.')
            carry_block:set_carrying(nil)
            om:destroy_entity(carrying)
         end
      end
   end
end
