local selector_util = require 'services.client.selection.selector_util'
local RulerWidget = require 'services.client.selection.ruler_widget'
local XZRegionSelector = class()
local Color4 = _radiant.csg.Color4
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local log = radiant.log.create_logger('xz_region_selector')

local DEFAULT_BOX_COLOR = Color4(192, 192, 192, 255)

local TERRAIN_NODES = 1

function XZRegionSelector:__init()
   self._mode = 'selection'
   self._require_supported = false
   self._require_unblocked = false
   self._show_rulers = true
   self._select_front_brick = true
   self._allow_select_cursor = false
   self._find_support_filter_fn = function(result)
      return self:_default_find_support_filter(result)
   end

   local identity_end_point_transform = function(p0, p1)
      return p0, p1
   end

   self._get_proposed_points_fn = identity_end_point_transform
   self._get_resolved_points_fn = identity_end_point_transform

   self:use_outline_marquee(DEFAULT_BOX_COLOR, DEFAULT_BOX_COLOR)
end

function XZRegionSelector:allow_select_cursor(allow)
   self._allow_select_cursor = allow
   return self
end

function XZRegionSelector:select_front_brick(v)
   self._select_front_brick = v
   return self
end

function XZRegionSelector:require_supported(supported)
   self._require_supported = supported
   return self
end

function XZRegionSelector:require_unblocked(unblocked)
   self._require_unblocked = unblocked
   return self
end

function XZRegionSelector:set_find_support_filter(filter_fn)
   self._find_support_filter_fn = filter_fn
   return self
end

-- used to constrain the selected region
-- examples include forcing the region to be square, enforcing minimum or maximum sizes,
-- or quantizing the region to certain step sizes
function XZRegionSelector:set_end_point_transforms(get_proposed_points_fn, get_resolved_points_fn)
   self._get_proposed_points_fn = get_proposed_points_fn
   self._get_resolved_points_fn = get_resolved_points_fn
   return self
end

function XZRegionSelector:set_show_rulers(value)
   self._show_rulers = value
   return self
end

function XZRegionSelector:done(cb)
   self._done_cb = cb
   return self
end

function XZRegionSelector:progress(cb)
   self._progress_cb = cb
   return self
end

function XZRegionSelector:fail(cb)
   self._fail_cb = cb
   return self
end

function XZRegionSelector:always(cb)
   self._always_cb = cb
   return self
end

function XZRegionSelector:_call_once(name, ...)
   local method_name = '_' .. name .. '_cb'
   if self[method_name] then
      local method = self[method_name]
      self[method_name] = nil
      method(self, ...)
   end
end

function XZRegionSelector:resolve(...)
   self:_call_once('done', ...)
   self:_call_once('always')
   -- If we've resolved, we can't possibly fail.
   self._fail_cb = nil
   self:_cleanup()
   return self
end

function XZRegionSelector:reject(...)
   self:_call_once('fail', ...)
   self:_call_once('always')
   -- If we've rejected, we can't possibly succeed.
   self._done_cb = nil
   self:_cleanup()
   return self
end

function XZRegionSelector:notify(...)
   if self._progress_cb then
      self._progress_cb(self, ...)
   end
   return self
end

function XZRegionSelector:_cleanup()
   stonehearth.selection:register_tool(self, false)

   self._fail_cb = nil
   self._progress_cb = nil
   self._done_cb = nil
   self._always_cb = nil

   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end

   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end

   if self._x_ruler then
      self._x_ruler:destroy()
      self._x_ruler = nil
   end   

   if self._z_ruler then
      self._z_ruler:destroy()
      self._z_ruler = nil
   end
end

function XZRegionSelector:destroy()
   self:reject('destroy')
end

-- set the 'can_contain_entity_filter'.  when growing the xz region,
-- make sure that it does *not* contain any of the entities for which
-- this filter returns false
function XZRegionSelector:set_can_contain_entity_filter(filter_fn)
   self._can_contain_entity_filter_fn = filter_fn
   return self
end

function XZRegionSelector:set_cursor(cursor)
   self._cursor = cursor
   return self
end

function XZRegionSelector:use_designation_marquee(color)
   self._create_node_fn = _radiant.client.create_designation_node
   self._box_color = color
   self._line_color = color
   return self
end

function XZRegionSelector:use_outline_marquee(box_color, line_color)
   self._create_node_fn = _radiant.client.create_selection_node
   self._box_color = box_color
   self._line_color = line_color
   return self
end

function XZRegionSelector:use_manual_marquee(marquee_fn)
   self._create_marquee_fn = marquee_fn
   return self
end

-- create the cube for the xz region given endpoints p0 and p1.
function XZRegionSelector:_create_cube(p0, p1)
   assert(p0 and p1)
   local min, max = Point3(p0), Point3(p1)
   for _, comp in ipairs({ 'x', 'y', 'z'}) do
      if min[comp] > max[comp] then
         min[comp], max[comp] = max[comp], min[comp]
      end
   end
   return Cube3(min, max + Point3.one)
end

-- return whether or not the given location is valid to be used in the creation
-- of the xz region.  if `check_containment_filter` is true, will also make sure
-- that all the entities at the specified poit pass the can_contain_entity_filter
-- filter.
--
function XZRegionSelector:_is_valid_location(brick, check_containment_filter)
   if self._require_unblocked and radiant.terrain.is_blocked(brick) then
      return false
   end
   if self._require_supported and not radiant.terrain.is_supported(brick) then
      return false
   end
   if check_containment_filter and self._can_contain_entity_filter_fn then
      local entities = radiant.terrain.get_entities_at_point(brick)
      for _, entity in pairs(entities) do
         if not self._can_contain_entity_filter_fn(entity, self) then
            return false
         end
      end
   end
   return true
end

-- get the brick under the screen coordinate (x, y) which is the best candidate
-- for adding to the xz region selector.  if `check_containment_filter` is true, will
-- also make sure that all the entities at the specified poit pass the can_contain_entity_filter
-- filter.
--
function XZRegionSelector:_get_hover_brick(x, y)
   local brick = selector_util.get_selected_brick(x, y, self._select_front_brick, function(result)
         return self._find_support_filter_fn(result, self)
      end)
   return brick
end

-- given a candidate p1, compute the p1 which would result in a valid xz region
-- will iterate from p0-p1 in 'i-major' order, where i is 'x' or 'z'.  See
-- _compute_p1 for more info
--
function XZRegionSelector:_compute_p1_loop(p0, p1, i)
   if not self:_is_valid_location(p0, true) then
      return nil
   end

   local j = i == 'x' and 'z' or 'x'
   local di = p1[i] > p0[i] and 1 or -1
   local dj = p1[j] > p0[j] and 1 or -1

   local pt = Point3(p0.x, p0.y, p0.z)

   while pt[i] ~= p1[i] + di do
      pt[j] = p0[j]
      while pt[j] ~= p1[j] + dj do
         if not self:_is_valid_location(pt, true) then
            local go_narrow = pt[i] == p0[i]
            if go_narrow then
               -- make the rect narrower and keep iterating
               p1[j] = pt[j] - dj
               break
            else
               -- go back 1 row and return
               pt[i] = pt[i] - di
               pt[j] = p1[j]
               return pt
            end
         end
         pt[j] = pt[j] + dj
      end
      pt[i] = pt[i] + di
   end
   return p1
end

-- given a candidate p1, compute the p1 which would result in a valid xz region.
--
function XZRegionSelector:_compute_p1(p0, p1)
   if not p0 or not p1 then
      return nil
   end

   local lx = math.abs(p1.x - p0.x)
   local lz = math.abs(p1.z - p0.z)

   local coord = lx > lz and 'x' or 'z'

   return self:_compute_p1_loop(p0, p1, coord)
end

function XZRegionSelector:_on_mouse_event(event)
   -- cancel on mouse button 2.
   if event and event:up(2) and not event.dragging then
      self:reject({ error = 'selection cancelled'})
      return
   end

   local current_brick = self:_get_hover_brick(event.x, event.y)
   -- TODO: clean this up
   if not event:up(1) and not event:down(1) then
      -- only recalculate when the brick changes
      if current_brick == self._last_hover_brick then
         return
      end
      self._last_hover_brick = current_brick
   end

   local valid_end_points = false
   local start, finish

   if current_brick then
      start = self._selected_p0 and self._p0 or current_brick
      finish = current_brick
      log:spam('hover bricks: %s, %s', tostring(start), tostring(finish))
      start, finish = self._get_proposed_points_fn(start, finish)
      log:spam('proposed bricks: %s, %s', tostring(start), tostring(finish))

      if self:_is_valid_location(start, true) then
         finish = self:_compute_p1(start, finish)
         start, finish = self._get_resolved_points_fn(start, finish)
         log:spam('resolved bricks: %s, %s', tostring(start), tostring(finish))
         valid_end_points = start and finish
      end
   end

   if valid_end_points then
      self._p0 = start
      self._p1 = finish
      self._selected_p0 = self._selected_p0 or event:down(1)
   else
      if not self._selected_p0 then
         self._p0 = nil
         self._p1 = nil
      else
         -- keep the last valid region
      end
   end

   local selected_cube = self._p0 and self._p1 and self:_create_cube(self._p0, self._p1)
   self:_notify_progress(selected_cube)
   self:_update_rulers(self._p0, self._p1)

   local done = self._selected_p0 and event:up(1)
   if done then
      self:resolve(selected_cube)
   end
end

function XZRegionSelector:_update_rulers(p0, p1)
   if not self._show_rulers or not self._x_ruler or not self._z_ruler then
      return
   end

   if p0 and p1 then
      -- if we're selecting the hover brick, the rulers are on the bottom of the selection
      -- if we're selecting the terrain brick, the rulers are on the top of the selection
      local y_offset = self._select_front_brick and 0 or 1
      local y = p0.y + y_offset
      local x_normal = Point3(0, 0, p0.z <= p1.z and 1 or -1)
      self._x_ruler:set_points(Point3(math.min(p0.x, p1.x), y, p1.z),
                               Point3(math.max(p0.x, p1.x), y, p1.z),
                               x_normal,
                               string.format('%d\'', math.abs(p1.x - p0.x) + 1))
      self._x_ruler:show()

      local z_normal = Point3(p0.x <= p1.x and 1 or -1, 0, 0)
      self._z_ruler:set_points(Point3(p1.x, y, math.min(p0.z, p1.z)),
                               Point3(p1.x, y, math.max(p0.z, p1.z)),
                               z_normal,
                               string.format('%d\'', math.abs(p1.z - p0.z) + 1))
      self._z_ruler:show()
   else
      self._x_ruler:hide()
      self._z_ruler:hide()
   end
end

function XZRegionSelector:_notify_progress(box)
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end

   if not box then
      return
   end

   if self._create_marquee_fn then
      self._render_node = self._create_marquee_fn(self, box)
   elseif self._create_node_fn then
      -- recreate the render node for the designation
      local region = Region2(Rect2(Point2(0, 0), 
                                   Point2(box.max.x - box.min.x, box.max.z - box.min.z)))
      self._render_node = self._create_node_fn(1, region, self._box_color, self._line_color)
                                    :set_position(box.min)
   end

   -- Why would we want a selectable cursor?  Because we're querying the actual displayed objects, and when
   -- laying down floor, we cut into the actual displayed object.  So, you select a piece of terrain, then cut
   -- into it, and then you move the mouse a smidge.  Now, the query goes through the new hole, hits another
   -- terrain block, and the hole _moves_ to the new selection; nudge the mouse again, and the hole jumps again.
   -- Outside of re-thinking the way selection works, this is the only fix that occurs to me.
   self._render_node:set_can_query(self._allow_select_cursor)
   self:notify(box)
end

function XZRegionSelector:_default_find_support_filter(result)
   local entity = result.entity

   -- TODO: maybe overthinking it, but perhaps 'return self._allow_nil_entity', and add that
   -- to the API?  The only case entity should be null is when allowing self-selection of the
   -- cursor.
   if not entity then
      return true
   end

   -- fast check for 'is terrain'
   if entity:get_id() == 1 then
      return true
   end

   -- solid regions are good if we're pointing at the top face
   if result.normal:to_int().y == 1 then
      local rcs = entity:get_component('region_collision_shape')
      if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
         return true
      end
   end

   -- otherwise, keep looking!
   return stonehearth.selection.FILTER_IGNORE
end

function XZRegionSelector:go()
   local box_color = self._box_color or DEFAULT_BOX_COLOR

   -- install a new mouse cursor if requested by the client.  this cursor
   -- will stick around until :destroy() is called on the selector!
   if self._cursor then
      self._cursor_obj = _radiant.client.set_cursor(self._cursor)
   end

   stonehearth.selection:register_tool(self, true)

   if self._show_rulers then
      self._x_ruler = RulerWidget()
      self._z_ruler = RulerWidget()
   end
   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 self:_on_mouse_event(e, e)
                                 return true
                              end)
   return self
end

return XZRegionSelector