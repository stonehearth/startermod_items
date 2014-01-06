local GotoEntityAction = class()

GotoEntityAction.name = 'goto entity'
GotoEntityAction.does = 'stonehearth:goto_entity'
GotoEntityAction.version = 1
GotoEntityAction.priority = 2
--TODO we need a scale to  describe relative importance
local log = radiant.log.create_logger('ai.goto_entity')

function GotoEntityAction:run(ai, entity, target)
   radiant.check.is_entity(target)

   log:debug('running goto entity action %s -> %s', entity, target)
   self._trace = radiant.entities.trace_location(target, 'goto target')
      :on_changed(function()
         ai:abort('target destination changed')
      end)
      :on_destroyed(function()
         ai:abort('target destination destroyed')
      end)

   local pathfinder = _radiant.sim.create_path_finder(entity, 'goto entity action')
                         :add_destination(target)

   log:debug('waiting for pathfinder to finish...')
   local path = ai:wait_for_path_finder(pathfinder)
   ai:execute('stonehearth:follow_path', path)
end

function GotoEntityAction:stop()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return GotoEntityAction
