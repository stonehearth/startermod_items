local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local WaitForCrateSpace = class()
WaitForCrateSpace.name = 'wait for crate space'
WaitForCrateSpace.does = 'stonehearth:wait_for_crate_space'
WaitForCrateSpace.args = {
   crate = Entity,        -- the crate that needs stuff
}
WaitForCrateSpace.think_output = {
   item_filter = 'function',  -- the filter function for crate items
}
WaitForCrateSpace.version = 2
WaitForCrateSpace.priority = 1

function WaitForCrateSpace:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()
   self._backpack = args.crate:get_component('stonehearth:backpack')

   self._space_listener = radiant.events.listen(args.crate, 'stonehearth:backpack:item_removed', self, self._on_space_available)
   self:_on_space_available()
end

function WaitForCrateSpace:_on_space_available()
   if not self._backpack then
      self._log:debug('crate destroyed')
      return
   end

   if self._backpack:is_full() then
      return
   end

   local filter_fn = self._backpack:get_filter()
   self._ai:set_think_output({
      item_filter = filter_fn
   })
end

function WaitForCrateSpace:stop_thinking(ai, entity)
   self._space_listener:destroy()
   self._space_listener = nil
end

return WaitForCrateSpace
