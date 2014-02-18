local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local WaitForFabricator = class()
WaitForFabricator.name = 'wait for fabricator'
WaitForFabricator.does = 'stonehearth:wait_for_fabricator'
WaitForFabricator.args = {
   fabricator = Fabricator,     -- the fabricator that needs stuff
}
WaitForFabricator.version = 2
WaitForFabricator.priority = 1

function WaitForFabricator:start_thinking(ai, entity, args)
   self._ai = ai
   self._fabricator = args.fabricator
   
   radiant.events.listen(self._fabricator, 'needs_work', self, self._on_needs_work)
   self:_on_needs_work(self._fabricator, self._fabricator, self._fabricator:needs_work())
end

function WaitForFabricator:_on_needs_work(fabricator, needs_work)
   if needs_work and not self._ready then
      self._ready = true
      self._ai:set_think_output()
   elseif not needs_work and self._ready then
      self._ready = false
      self._ai:clear_think_output()
   end
end

function WaitForFabricator:stop_thinking(ai, entity, args)
   radiant.events.unlisten(self._fabricator, 'needs_work', self, self._on_needs_work)
   self._ready = false
end

return WaitForFabricator
