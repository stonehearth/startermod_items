-- helper methods for selectors
local SelectorUtil = {}
local Point3 = _radiant.csg.Point3

-- given the x, y coordinate on screen, return the brick that is a
-- candidate for selection.  trace a ray through that point in the screen,
-- ignoring nodes between ourselves and the desired brick (e.g. the cursor
-- is almost certainly going to get in the way!)
--
function SelectorUtil.get_selected_brick(x, y, filter_fn)
   assert(filter_fn)

   -- query the scene to figure out what's under the mouse cursor
   local s = _radiant.client.query_scene(x, y)
   for result in s:each_result() do
      local filter_result = filter_fn(result)
      if filter_result == stonehearth.selection.FILTER_IGNORE then
         -- keep going...
      elseif filter_result == true then
         return result.brick
      else
         -- everything else (including false and nil) indicates failure
         return nil
      end
   end
end

return SelectorUtil
