local StonehearthTrees = {}
local TreeTracker = radiant.mods.require('/stonehearth_trees/lib/tree_tracker.lua')
local singleton = {
   trackers = {}
}

function StonehearthTrees.find_path_to_tree(from, cb)
   local faction = ''
   local unit_info = from:get_component('unit_info')
   if unit_info then
      faction = unit_info:get_faction()
   end
   return _get_tree_tracker(faction):find_path_to_tree(from, cb)
end

function StonehearthTrees.harvest_tree(faction, tree)
   local tracker = singleton.trackers[faction]
   if not tracker then
      tracker = TreeTracker(faction)
      singleton.trackers[faction] = tracker
   end
   _get_tree_tracker(faction):harvest_tree(tree)
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
