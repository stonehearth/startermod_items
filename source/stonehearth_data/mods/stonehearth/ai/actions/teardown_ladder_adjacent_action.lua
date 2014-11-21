local Entity = _radiant.om.Entity
local LadderBuilder = require 'services.server.build.ladder_builder'

local TeardownLadderAdjacent = class()
TeardownLadderAdjacent.name = 'teardown ladder adjacent'
TeardownLadderAdjacent.does = 'stonehearth:teardown_ladder_adjacent'
TeardownLadderAdjacent.args = {
   ladder = Entity,           -- the ladder to build
   builder = LadderBuilder,   -- the ladder builder class
}
TeardownLadderAdjacent.version = 2
TeardownLadderAdjacent.priority = 1

function TeardownLadderAdjacent:start_thinking(ai, entity, args)
   local builder = args.builder
   if builder:is_ladder_finished() then
      -- if the ladder's already finished, there's nothing to do
      return
   end

   local material = builder:get_material()
   if ai.CURRENT.carrying and not radiant.entities.is_material(ai.CURRENT.carrying, material) then
      return
   end
   ai:set_think_output()
end

function TeardownLadderAdjacent:run(ai, entity, args)
   local ladder = args.ladder
   local builder = args.builder
   local carrying = radiant.entities.get_carrying(entity)
   local item_component

   local function full()
      if not item_component and carrying then
         item_component = carrying:get_component('item')
      end
      if item_component then
         return item_component:get_stacks() == item_component:get_max_stacks()
      end
      return false
   end
   
   ai:unprotect_entity(args.ladder)
   while not full() and not builder:is_ladder_finished() do
      radiant.entities.turn_to_face(entity, ladder)
      ai:execute('stonehearth:run_effect', { effect = 'work' })
      builder:shrink_ladder()

      if not radiant.entities.increment_carrying(entity) then
         local oak_log = radiant.entities.create_entity('stonehearth:resources:wood:oak_log')
         radiant.entities.pickup_item(entity, oak_log)
         oak_log:add_component('item')
                     :set_stacks(1)
      end
   end
end

function TeardownLadderAdjacent:start(ai, entity)
   ai:set_status_text('removing ladder...')
end

return TeardownLadderAdjacent
