local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local WaitForCrateSpace = class()
WaitForCrateSpace.name = 'wait for storage space'
WaitForCrateSpace.does = 'stonehearth:wait_for_storage_space'
WaitForCrateSpace.args = {
   storage = Entity,          -- the storage that needs stuff
}
WaitForCrateSpace.think_output = {
   item_filter = 'function',  -- the filter function for storage items
}
WaitForCrateSpace.version = 2
WaitForCrateSpace.priority = 1

function WaitForCrateSpace:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()
   self._storage = args.storage:get_component('stonehearth:storage')
   self._ready = false

   self._more_space_listener = radiant.events.listen(args.storage, 'stonehearth:storage:item_removed', self, self._on_space_changed)
   self._less_space_listener = radiant.events.listen(args.storage, 'stonehearth:storage:item_added', self, self._on_space_changed)
   self:_on_space_changed()
end

function WaitForCrateSpace:_on_space_changed()
   if not self._storage then
      self._log:debug('storage destroyed')
      return
   end

   if self._storage:is_full() and self._ready then
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
