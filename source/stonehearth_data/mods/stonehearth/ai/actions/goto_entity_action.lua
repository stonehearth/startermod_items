local GotoEntityAction = class()

GotoEntityAction.name = 'goto entity'
GotoEntityAction.does = 'stonehearth:goto_entity'
GotoEntityAction.priority = 2
--TODO we need a scale to  describe relative importance

function GotoEntityAction:run(ai, entity, target)
   radiant.check.is_entity(target)
   self._trace = radiant.entities.trace_location(target, 'goto target')
      :on_changed(function()
         ai:abort('target destination changed')
      end)
      :on_destroyed(function()
         ai:abort('target destination destroyed')
      end)

   local pathfinder = _radiant.sim.create_path_finder('goto entity action')
                         :set_source(entity)
                         :add_destination(target)

   local path = ai:wait_for_path_finder(pathfinder)
   ai:execute('stonehearth:follow_path', path)
end

function GotoEntityAction:stop()
   if self._trace then
      self._trace:stop()
      self._trace = nil
   end
end

return GotoEntityAction
