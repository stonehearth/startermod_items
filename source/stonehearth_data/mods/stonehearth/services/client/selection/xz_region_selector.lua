local selector_util = require 'services.client.selection.selector_util'
local RulerWidget = require 'services.client.selection.ruler_widget'
local XZRegionSelector = class()
local Color4 = _radiant.csg.Color4
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3

local log = radiant.log.create_logger('xz_region_selector')

local DEFAULT_BOX_COLOR = Color4(192, 192, 192, 255)

local TERRAIN_NODES = 1

function XZRegionSelector:__init()
   self._state = 'start'
   self._ruler_color_valid = Color4(0, 0, 0, 128)
   self._ruler_color_invalid = Color4(255, 0, 0, 128)
   self._require_supported = false
   self._require_unblocked = false
   self._show_rulers = true
   self._min_size = 0
   self._max_size = radiant.math.MAX_INT32
   self._select_front_brick = true
   self._validation_offset = Point3(0, 0, 0)
   self._allow_select_cursor = false
   self._invalid_cursor = 'stonehearth:cursors:invalid_hover'
   self._valid_region_cache = Region3()

   self._find_support_filter_fn = stonehearth.selection.find_supported_xz_region_filter

   local identity_end_point_transform = function(q0, q1)
      if not q0 or not q1 then
         return nil, nil
      end
      return Point3(q0), Point3(q1)   -- return a copy to be safe
   end

   self._get_proposed_points_fn = identity_end_point_transform
   self._get_resolved_points_fn = identity_end_point_transform

   self._cursor_fn = function(selected_cube, stabbed_normal)
      if not selected_cube then
         return self._invalid_cursor
      end
      return self._cursor
   end

   self._last_ignored_entities = {}
   self._ignored_entities = {}

   self:_initialize_dispatch_table()

   self:use_outline_marquee(DEFAULT_BOX_COLOR, DEFAULT_BOX_COLOR)
end

function XZRegionSelector:set_min_size(value)
   self._min_size = value
   return self
end

function XZRegionSelector:set_max_size(value)
   self._max_size = value
   return self
end

function XZRegionSelector:set_show_rulers(value)
   self._show_rulers = value
   return self
end

function XZRegionSelector:select_front_brick(value)
   self._select_front_brick = value
   return self
end

-- ugly parameter used to validate a different region than the selected region
function XZRegionSelector:set_validation_offset(value)
   self._validation_offset = value
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

-- set the 'can_contain_entity_filter'.  when growing the xz region,
-- make sure that it does *not* contain any of the entities for which
-- this filter returns false
function XZRegionSelector:set_can_contain_entity_filter(filter_fn)
   self._can_contain_entity_filter_fn = filter_fn
   return self
end

function XZRegionSelector:set_ghost_ignored_entity_filter(filter_fn)
   self._ghost_ignored_entity_filter_fn = filter_fn
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

function XZRegionSelector:allow_select_cursor(allow)
   self._allow_select_cursor = allow
   return self
end

function XZRegionSelector:set_cursor(cursor)
   self._cursor = cursor
   return self
end

function XZRegionSelector:set_invalid_cursor(invalid_cursor)
   self._invalid_cursor = invalid_cursor
   return self
end

function XZRegionSelector:set_cursor_fn(cursor_fn)
   self._cursor_fn = cursor_fn
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

   self:_restore_ignored_entities()

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
-- of the xz region.
function XZRegionSelector:_is_valid_location(brick)
   if not brick then
      return false
   end

   if self._require_unblocked and radiant.terrain.is_blocked(brick) then
      return false
   end
   if self._require_supported and not radiant.terrain.is_supported(brick) then
      return false
   end
   if self._can_contain_entity_filter_fn then
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
function XZRegionSelector:_get_brick_at(x, y)
   local brick, normal = selector_util.get_selected_brick(x, y, function(result)
         local entity = result.entity

         -- The only case entity should be null is when allowing self-selection of the cursor.
         if self._allow_select_cursor and not entity then
            return true
         end

         if not entity then
            return stonehearth.selection.FILTER_IGNORE 
         end

         local render_entity = _radiant.client.get_render_entity(entity)
         if render_entity and render_entity:has_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE) then
            return stonehearth.selection.FILTER_IGNORE
         end

         local valid_support = self._find_support_filter_fn(result, self)

         if valid_support == stonehearth.selection.FILTER_IGNORE then
            if self._ghost_ignored_entity_filter_fn and self._ghost_ignored_entity_filter_fn(entity) then
               self:_add_to_ignored_entities(entity)            
            end
         end

         return valid_support
      end)
   return brick, normal
end

-- Given a candidate p1, compute the 'best' p1 which results in a valid xz region.
-- The basic algorithm is simple:
--     1) For each row, scan until you reach an invalid point or the x limit of a previous row.
--     2) Add each valid point to a region.
--     3) Return the point in the region closest to p1.
-- The rest of the code is just optimization and bookkeeping.
function XZRegionSelector:_compute_endpoint(q0, q1)
   if not q0 or not q1 then
      return nil
   end

   -- if q0 has changed, invalidate our cache
   if q0 ~= self._valid_region_origin then
      self._valid_region_cache:clear()
      self._valid_region_origin = Point3(q0)
   end

   -- if the endpoints are already validated, then the whole cube is valid
   if self._valid_region_cache:contains(q0) and self._valid_region_cache:contains(q1) then
      return q1
   end

   if not self:_is_valid_location(q0) then
      return nil
   end

   local dx = q1.x > q0.x and 1 or -1
   local dz = q1.z > q0.z and 1 or -1
   local r0 = Point3(q0) -- row start point
   local r1 = Point3(q0) -- row end point
   local limit_x = q1.x
   local valid_x, start_x

   -- iterate over all rows
   for j = q0.z, q1.z, dz do
      if not limit_x then
         -- row is completely obstructed, no further rows can be valid
         break
      end

      r0.z = j
      r1.z = j

      r1.x = limit_x
      local unverified_region = Region3(self:_create_cube(r0, r1))
      unverified_region:subtract_region(self._valid_region_cache)

      if not unverified_region:empty() then
         local valid_x = nil
         local start_x = unverified_region:get_closest_point(q0).x

         -- if we're not at the row start, valid_x was the previous point
         if start_x ~= r0.x then
            valid_x = start_x - dx
         end

         -- iterate over the untested columns in the row
         for i = start_x, limit_x, dx do
            r1.x = i
            if self:_is_valid_location(r1) then
               valid_x = i
            else
               -- the new limit is the last valid x value
               limit_x = valid_x
               break
            end
         end

         if valid_x then
            -- add the row to the valid_region
            r1.x = valid_x
            local valid_row = self:_create_cube(r0, r1)
            self._valid_region_cache:add_cube(valid_row)
         end
      end
   end

   self._valid_region_cache:optimize_by_merge()
   local resolved_q1 = self._valid_region_cache:get_closest_point(q1)
   return resolved_q1
end

function XZRegionSelector:_find_valid_region(q0, q1)
   if not q0 or not q1 then
      return nil, nil
   end

   -- validation offset is an annoying hack used to validate regions that are offset from the selected region
   -- used for things like mining zones, roads, and floors
   local offset = self._validation_offset
   local v0, v1 = q0 + offset, q1 + offset

   v1 = self:_compute_endpoint(v0, v1)
   if not v1 then
      return nil, nil
   end
   
   q1 = v1 - offset

   return q0, q1
end

function XZRegionSelector:_initialize_dispatch_table()
   self._dispatch_table = {
      start       = self._run_start_state,
      p0_selected = self._run_p0_selected_state,
      stop        = self._run_stop_state
   }
end

function XZRegionSelector:_update()
   if not self._action then
      return
   end

   if self._action == 'reject' then
      self:reject({ error = 'selection cancelled' }) -- is this still the correct argument?
      return
   end

   local selected_cube = self._p0 and self._p1 and self:_create_cube(self._p0, self._p1)

   self:_update_selected_cube(selected_cube)
   self:_update_rulers(self._p0, self._p1)
   self:_update_cursor(selected_cube, self._stabbed_normal)
   self:_update_ignored_entities()

   if self._action == 'notify' then
      self:notify(selected_cube)
   elseif self._action == 'resolve' then
      self:resolve(selected_cube)
   else
      log:error('uknown action: %s', self._action)
      assert(false)
   end
end

function XZRegionSelector:_resolve_endpoints(q0, q1, stabbed_normal)
   --log:spam('selected endpoints: %s, %s', tostring(q0), tostring(q1))

   q0, q1 = self._get_proposed_points_fn(q0, q1, stabbed_normal)
   --log:spam('proposed endpoints: %s, %s', tostring(q0), tostring(q1))

   q0, q1 = self:_find_valid_region(q0, q1)
   --log:spam('validated endpoints: %s, %s', tostring(q0), tostring(q1))

   q0, q1 = self:_limit_dimensions(q0, q1)
   --log:spam('bounded endpoints: %s, %s', tostring(q0), tostring(q1))

   q0, q1 = self._get_resolved_points_fn(q0, q1, stabbed_normal)
   --log:spam('resolved endpoints: %s, %s', tostring(q0), tostring(q1))

   if not q0 or not q1 then
      return nil, nil
   end

   return q0, q1
end

function XZRegionSelector:_limit_dimensions(q0, q1)
   if not q0 or not q1 then
      return nil, nil
   end

   local new_q1 = Point3(q1)
   local size = self:_create_cube(q0, q1):get_size()

   if size.x > self._max_size then
      local sign = q1.x >= q0.x and 1 or -1
      new_q1.x = q0.x + sign*(self._max_size-1)
   end

   if size.z > self._max_size then
      local sign = q1.z >= q0.z and 1 or -1
      new_q1.z = q0.z + sign*(self._max_size-1)
   end

   return q0, new_q1
end

function XZRegionSelector:_on_mouse_event(event)
   -- This is the action that will be taken in _update() unless specified otherwise
   self._action = 'notify'

   local brick, normal = self:_get_brick_at(event.x, event.y)

   if brick and self._select_front_brick then
      -- get the brick in front of the stabbed brick
      brick = brick + normal
   end

   local state_transition_fn = self._dispatch_table[self._state]
   assert(state_transition_fn)

   -- Given the inputs and the current state, get the next state
   local next_state = state_transition_fn(self, event, brick, normal)
   self:_update()

   self._last_brick = brick
   self._state = next_state
end

function XZRegionSelector:_run_start_state(event, brick, normal)
   if stonehearth.selection.user_cancelled(event) then
      self._action = 'reject'
      return 'stop'
   end

   if not brick then
      self._p0, self._p1 = nil, nil
      return 'start'
   end

   -- avoid spamming recalculcation and update if nothing has changed
   if brick == self._last_brick and not event:down(1) then
      self._action = nil
      return 'start'
   end

   local q0, q1 = self:_resolve_endpoints(brick, brick, normal)

   if not q0 or not q1 then
      self._p0, self._p1 = nil, nil
      return 'start'
   end

   self._p0, self._p1 = q0, q1
   self._stabbed_normal = normal

   if event:down(1) then
      return 'p0_selected'
   else
      return 'start'
   end
end

function XZRegionSelector:_run_p0_selected_state(event, brick, normal)
   if stonehearth.selection.user_cancelled(event) then
      self._action = 'reject'
      return 'stop'
   end

   -- avoid spamming recalculcation and update if nothing has changed
   if brick == self._last_brick and not event:up(1) then
      self._action = nil
      return 'p0_selected'
   end

   local q0, q1 = self:_resolve_endpoints(self._p0, brick, self._stabbed_normal)

   if not q0 or not q1 then
      -- maybe the world has changed after we started dragging
      log:error('unable to resolve endpoints: %s, %s', tostring(q0), tostring(q1))
      return 'p0_selected'
   end

   self._p0, self._p1 = q0, q1

   if event:up(1) then
      -- Make sure our cache still reflects the current state of the world.
      -- Note we still have a short race condition with the server as the state
      -- may have changed there which is not yet reflected on the client.
      self._valid_region_cache:clear()
      local q0, q1 = self:_find_valid_region(self._p0, self._p1)
      local is_valid_region = q0 == self._p0 and q1 == self._p1

      if is_valid_region and self:_are_valid_dimensions(self._p0, self._p1) then
         self._action = 'resolve'
      else
         self._action = 'reject'
      end
      return 'stop'
   else
      return 'p0_selected'
   end
end

function XZRegionSelector:_run_stop_state(event, brick, normal)
   -- waiting to be destroyed
   return 'stop'
end

function XZRegionSelector:_is_valid_length(length)
   local valid = length >= self._min_size and length <= self._max_size
   return valid
end

function XZRegionSelector:_are_valid_dimensions(p0, p1)
   local size = self:_create_cube(p0, p1):get_size()
   local valid = self:_is_valid_length(size.x) and self:_is_valid_length(size.z)
   return valid
end

function XZRegionSelector:_update_rulers(p0, p1)
   if not self._show_rulers or not self._x_ruler or not self._z_ruler then
      return
   end

   if p0 and p1 then
      -- if we're selecting the hover brick, the rulers are on the bottom of the selection
      -- if we're selecting the terrain brick, the rulers are on the top of the selection
      local offset = self._select_front_brick and Point3.zero or Point3.unit_y
      local q0, q1 = p0 + offset, p1 + offset

      self:_update_ruler(self._x_ruler, q0, q1, 'x')
      self._x_ruler:show()

      self:_update_ruler(self._z_ruler, q0, q1, 'z')
      self._z_ruler:show()
   else
      self._x_ruler:hide()
      self._z_ruler:hide()
   end
end

function XZRegionSelector:_update_ruler(ruler, p0, p1, dimension)
   local d = dimension
   local dn = d == 'x' and 'z' or 'x'
   local min = math.min(p0[d], p1[d])
   local max = math.max(p0[d], p1[d])
   local length = max - min + 1
   local color = self:_is_valid_length(length) and self._ruler_color_valid or self._ruler_color_invalid
   ruler:set_color(color)

   local min_point = Point3(p1)
   min_point[d] = min

   local max_point = Point3(p1)
   max_point[d] = max

   -- don't use Point3.zero since we need it to be mutable
   local normal = Point3(0, 0, 0)
   normal[dn] = p0[dn] <= p1[dn] and 1 or -1

   ruler:set_points(min_point, max_point, normal, string.format('%d\'', length))
end

function XZRegionSelector:_update_selected_cube(box)
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
      local size = box:get_size()
      local region = Region2(Rect2(Point2.zero, Point2(size.x, size.z)))
      self._render_node = self._create_node_fn(1, region, self._box_color, self._line_color)
                                    :set_position(box.min)
   end

   -- Why would we want a selectable cursor?  Because we're querying the actual displayed objects, and when
   -- laying down floor, we cut into the actual displayed object.  So, you select a piece of terrain, then cut
   -- into it, and then you move the mouse a smidge.  Now, the query goes through the new hole, hits another
   -- terrain block, and the hole _moves_ to the new selection; nudge the mouse again, and the hole jumps again.
   -- Outside of re-thinking the way selection works, this is the only fix that occurs to me.
   self._render_node:set_can_query(self._allow_select_cursor)
end

function XZRegionSelector:_update_cursor(box, stabbed_normal)
   local cursor = self._cursor_fn and self._cursor_fn(box, stabbed_normal)

   if cursor == self._current_cursor then
      return
   end

   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end

   if cursor then
      self._cursor_obj = _radiant.client.set_cursor(cursor)
   end
end

function XZRegionSelector:_update_ignored_entities()
   if not self._ghost_ignored_entity_filter_fn then
      return
   end

   -- ghost entities that are new to the ignored_entities set
   self:_each_item_not_in_map(self._ignored_entities, self._last_ignored_entities, function(item)
         self:_set_ghost_mode(item.entity)
      end)

   -- unghost entities that have left the ignored_entities set
   self:_each_item_not_in_map(self._last_ignored_entities, self._ignored_entities, function(item)
         self:_set_entity_material(item.entity, item.material)
      end)

   self._last_ignored_entities = self._ignored_entities
   self._ignored_entities = {}
end

-- calls fn for each item in map that is not in the reference_map
function XZRegionSelector:_each_item_not_in_map(map, reference_map, fn)
   for id, item in pairs(map) do
      if not reference_map[id] then
         fn(item)
      end
   end
end

function XZRegionSelector:_add_to_ignored_entities(entity)
   local id = entity:get_id()
   local value = self._last_ignored_entities[id]

   if not value then
      local render_entity = _radiant.client.get_render_entity(entity)
      local material = render_entity:get_material_override()
      value = {
         entity = entity,
         material = material
      }
   end

   self._ignored_entities[id] = value
end

function XZRegionSelector:_restore_ignored_entities()
   for id, item in pairs(self._last_ignored_entities) do
      self:_set_entity_material(item.entity, item.material)
   end
end

function XZRegionSelector:_set_ghost_mode(entity, ghost_mode)
   if not entity:is_valid() then
      return
   end

   local render_entity = _radiant.client.get_render_entity(entity)
   local material = render_entity:get_material_path('hud')
   if material and material ~= '' then
      render_entity:set_material_override(material)
   end
end

function XZRegionSelector:_set_entity_material(entity, material)
   if not entity:is_valid() then
      return
   end

   local render_entity = _radiant.client.get_render_entity(entity)
   render_entity:set_material_override(material)
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
                                 self:_on_mouse_event(e)
                                 return true
                              end)

   -- TODO: want to be able to call this
   -- self._input_capture:push_object_state()

   return self
end

return XZRegionSelector
