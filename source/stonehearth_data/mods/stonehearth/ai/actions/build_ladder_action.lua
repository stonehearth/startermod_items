local Entity = _radiant.om.Entity
local LadderBuilder = require 'services.server.build.ladder_builder'

local BuildLadder = class()
BuildLadder.name = 'build ladder'
BuildLadder.does = 'stonehearth:build_ladder'
BuildLadder.args = {
   ladder = Entity,           -- the ladder to build
   builder = LadderBuilder,   -- the ladder builder class
}
BuildLadder.version = 2
BuildLadder.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(BuildLadder)
         :execute('stonehearth:pickup_item_made_of', { material = ai.ARGS.builder:get_material() })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.ladder })
         :execute('stonehearth:build_ladder_adjacent', { ladder = ai.ARGS.ladder, builder = ai.ARGS.builder })
