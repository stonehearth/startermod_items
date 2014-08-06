local Entity = _radiant.om.Entity
local LadderBuilder = require 'services.server.build.ladder_builder'

local TeardownLadder = class()
TeardownLadder.name = 'teardown ladder'
TeardownLadder.does = 'stonehearth:teardown_ladder'
TeardownLadder.args = {
   ladder = Entity,           -- the ladder to build
   builder = LadderBuilder,   -- the ladder builder class
}
TeardownLadder.version = 2
TeardownLadder.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(TeardownLadder)
         :execute('stonehearth:drop_carrying_if_stacks_full', {})
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.ladder })
         :execute('stonehearth:teardown_ladder_adjacent', { ladder = ai.ARGS.ladder, builder = ai.ARGS.builder })
         :execute('stonehearth:drop_carrying_if_stacks_full', {})
