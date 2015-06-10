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
   self._storage = args.crate:get_component('stonehearth:storage_filter')
   self._ready = false

   self._more_space_listener = radiant.events.listen(args.crate, 'stonehearth:backpack:item_removed', self, self._on_space_changed)
   self._less_space_listener = radiant.events.listen(args.crate, 'stonehearth:backpack:item_added', self, self._on_space_changed)
   self:_on_space_changed()
end

function WaitForCrateSpace:_on_space_changed()
   if not self._backpack then
      self._log:debug('crate destroyed')
      return
   end

   if self._backpack:is_full() and self._ready then
      self._ready = false
      self._ai:clear_think_output()
      return
   end

   if not self._ready then
      self._ready = true
      local filter_fn = self._storage:get_filter_function()
      self._ai:set_think_output({
         item_filter = filter_fn
      })
   end
end

function WaitForCrateSpace:stop_thinking(ai, entity)
   self._more_space_listener:destroy()
   self._more_space_listener = nil

   self._less_space_listener:destroy()
   self._less_space_listener = nil
end

return WaitForCrateSpace
