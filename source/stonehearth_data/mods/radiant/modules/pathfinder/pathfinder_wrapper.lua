local PathFinderWrapper = class()

function PathFinderWrapper:__init(pf)
   self._pf = pf
end

function PathFinderWrapper:set_source(s)
   self._pf:set_source(s)
   return self
end

function PathFinderWrapper:add_destination(d)
   self._pf:add_destination(d)
   return self
end

function PathFinderWrapper:remove_destination(d)
   self._pf:remove_destination(d)
   return self
end

function PathFinderWrapper:set_solved_cb(cb)
   self._pf:set_solved_cb(cb)
   return self
end

function PathFinderWrapper:set_filter_fn(fn)
   self._pf:set_filter_fn(fn)
   return self
end

function PathFinderWrapper:find_closest_dst()
   radiant.pathfinder._track_world_items(self._pf)
   return self
end

function PathFinderWrapper:start()
   return self._pf:start()
end

function PathFinderWrapper:restart()
   return self._pf:restart()
end

function PathFinderWrapper:stop()
   return self._pf:stop()
end

function PathFinderWrapper:get_solution()
   return self._pf:get_solution()
end

function PathFinderWrapper:is_idle()
   return self._pf:is_idle(d)
end


return PathFinderWrapper
