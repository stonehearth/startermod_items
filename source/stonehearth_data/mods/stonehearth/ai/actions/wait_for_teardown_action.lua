local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local WaitForTeardown = class()
WaitForTeardown.name = 'wait for teardown'
WaitForTeardown.does = 'stonehearth:wait_for_teardown'
WaitForTeardown.args = {
   fabricator = Fabricator,     -- the fabricator that needs stuff
}
WaitForTeardown.version = 2
WaitForTeardown.priority = 1

function WaitForTeardown:start_thinking(ai, entity, args)
   self._ai = ai
   self._fabricator = args.fabricator
   self._read = false
   
   radiant.events.listen(self._fabricator, 'needs_teardown', self, self._on_needs_teardown)
   self:_on_needs_teardown(self._fabricator, self._fabricator, self._fabricator:needs_teardown())
end

function WaitForTeardown:_on_needs_teardown(fabricator, needs_teardown)
   if needs_teardown and not self._ready then
      self._ready = true
      self._ai:set_think_output()
   elseif not needs_teardown and self._ready then
      self._ready = false
      self._ai:clear_think_output()
   end
end

function WaitForTeardown:stop_thinking(ai, entity, args)
   radiant.events.unlisten(self._fabricator, 'needs_teardown', self, self._on_needs_teardown)
   self._ready = false
end

return WaitForTeardown
