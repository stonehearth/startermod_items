local selector_util = require 'services.client.selection.selector_util'
local XZRegionSelector = class()
local Color4 = _radiant.csg.Color4
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local DEFAULT_BOX_COLOR = Color4(192, 192, 192, 255)

local TERRAIN_NODES = 1

function XZRegionSelector:__init()
   self._mode = 'selection'
   self._require_supported = false
   self._require_unblocked = false
   self._find_support_filter_fn = function(result)
      return self:_default_find_support_filter(result)
   end

   self:use_outline_marquee(DEFAULT_BOX_COLOR, DEFAULT_BOX_COLOR)
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

function XZRegionSelector:deactivate_tool()
   self:destroy()  
end

function XZRegionSelector:destroy()
   stonehearth.selection:register_tool(self, false)

   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end
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
function XZRegionSelector:_get_hover_brick(x, y, check_containment_filter)
   local brick = selector_util.get_selected_brick(x, y, function(result)
         return self._find_support_filter_fn(result, self)
      end)
   if brick and self:_is_valid_location(brick, check_containment_filter) then
      return brick
   end
end

-- given a candidate p1, compute the p1 which would result in a valid xz region
-- will iterate from p0-p1 in 'i-major' order, where i is 'x' or 'z'.  See
-- _compute_p1 for more info
--
function XZRegionSelector:_compute_p1_loop(p1, i)
   local j = i == 'x' and 'z' or 'x'
   local di = p1[i] > self._p0[i] and 1 or -1
   local dj = p1[j] > self._p0[j] and 1 or -1

   local pt = Point3(self._p0.x, self._p0.y, self._p0.z)

   while pt[i] ~= p1[i] + di do
      pt[j] = self._p0[j]
      while pt[j] ~= p1[j] + dj do
         if not self:_is_valid_location(pt, true) then
            local go_narrow = pt[i] == self._p0[i]
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
function XZRegionSelector:_compute_p1(p1)
   local lx = math.abs(p1.x - self._p0.x)
   local lz = math.abs(p1.z - self._p0.z)

   local coord = lx > lz and 'x' or 'z'

   return self:_compute_p1_loop(p1, coord)
end

function XZRegionSelector:_on_mouse_event(event)
   -- cancel on mouse button 2.
   if event and event:up(2) and not event.dragging then
      if self._fail_cb then
         self._fail_cb(self)
      end
      if self._always_cb then
         self._always_cb(self)
      end
      self:destroy()
      return
   end

   local current_brick = self:_get_hover_brick(event.x, event.y, not self._finished_p0)
   if current_brick then
      if not self._finished_p0 then
         -- select p0...
         self._p0 = current_brick
         self._p1 = self:_compute_p1(current_brick)
         self._finished_p0 = event:down(1)
      end
      if self._finished_p0 then
         -- select p1...
         self._p1 = self:_compute_p1(current_brick)
         self._finished = event:up(1)
      end
   end

   if self._p0 then
      local selected_cube = self:_create_cube(self._p0, self._p1)
      if self._finished then
         if self._done_cb then
            self._done_cb(self, selected_cube)
         end
         self:destroy()
      else
         self:_notify_progress(selected_cube)
      end
   end
end

function XZRegionSelector:_notify_progress(box)
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
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
   self._render_node:set_can_query(false)
   if self._progress_cb then
      self._progress_cb(self, box)
   end   
end

function XZRegionSelector:_default_find_support_filter(result)
   local entity = result.entity

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

   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 self:_on_mouse_event(e, e)
                                 return true
                              end)
   return self
end

return XZRegionSelector