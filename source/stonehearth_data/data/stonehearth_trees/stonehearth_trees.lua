local StonehearthTrees = {}
local TreeTracker = require 'lib.tree_tracker'
local singleton = {
   trackers = {}
}

function StonehearthTrees.find_path_to_tree(from, cb)
   local faction = _get_faction(from)
   return _get_tree_tracker(faction):find_path_to_tree(from, cb)
end

function StonehearthTrees.stop_searching(from)
   local faction = _get_faction(from)
   _get_tree_tracker(faction):stop_search(from)
end

function StonehearthTrees.harvest_tree(faction, tree)
   local tracker = singleton.trackers[faction]
   if not tracker then
      tracker = TreeTracker(faction)
      singleton.trackers[faction] = tracker
   end
   _get_tree_tracker(faction):harvest_tree(tree)
end

function _get_faction(entity)
   local faction = ''
   local unit_info = entity:get_component('unit_info')
   if unit_info then
      faction = unit_info:get_faction()
   end
   return faction
end

function _get_tree_tracker(faction)
   local tracker = singleton.trackers[faction]
   if not tracker then
      tracker = TreeTracker(faction)
      singleton.trackers[faction] = tracker
   end
   return tracker
end

return StonehearthTrees
