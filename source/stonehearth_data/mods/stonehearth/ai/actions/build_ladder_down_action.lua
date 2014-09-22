local Entity = _radiant.om.Entity
local LadderBuilder = require 'services.server.build.ladder_builder'

local BuildLadderDown = class()
BuildLadderDown.name = 'build ladder'
BuildLadderDown.does = 'stonehearth:build_ladder_down'
BuildLadderDown.args = {
   ladder = Entity,           -- the ladder to build
   builder = LadderBuilder,   -- the ladder builder class
}
BuildLadderDown.version = 2
BuildLadderDown.priority = 1

function BuildLadderDown:start_thinking(ai, entity, args)
   local ladder = args.ladder

   local location = radiant.entities.get_world_grid_location(ladder)
   if not location then
      return
   end

   local height  = ladder:get_component('stonehearth:ladder')
                              :get_desired_height()

   location.y = location.y + height
   ai:set_think_output({
         ladder_top = location
      })
end

local ai = stonehearth.ai
return ai:create_compound_action(BuildLadderDown)
         :execute('stonehearth:pickup_item_made_of', { material = ai.ARGS.builder:get_material() })
         :execute('stonehearth:create_proxy_entity', {
               location = ai.BACK(2).ladder_top,
               reason = 'build ladder down',
               use_default_adjacent_region = true
            })
         :execute('stonehearth:goto_entity', { entity = ai.PREV.entity })
         :execute('stonehearth:build_ladder_adjacent', { ladder = ai.ARGS.ladder, builder = ai.ARGS.builder, direction = 'down' })

