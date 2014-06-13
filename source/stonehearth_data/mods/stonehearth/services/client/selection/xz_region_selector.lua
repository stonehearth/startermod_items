local XZRegionSelector = class()
local Color4 = _radiant.csg.Color4
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local DEFAULT_BOX_COLOR = Color4(192, 192, 192, 255)

local TERRAIN_NODES = 1

function XZRegionSelector:__init()
   self._mode = 'selection'
   self._selection_flags = TERRAIN_NODES

   self:use_outline_marquee(DEFAULT_BOX_COLOR, DEFAULT_BOX_COLOR)
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

   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   if self._cursor_obj then
      self._cursor_obj:destroy()
      self._cursor_obj = nil
   end
   if self._xz_selector then
      self._xz_selector:destroy()
      self._xz_selector = nil
   end
end

function XZRegionSelector:go()
   local box_color = self._box_color or DEFAULT_BOX_COLOR

   -- install a new mouse cursor if requested by the client.  this cursor
   -- will stick around until :destroy() is called on the selector!
   if self._cursor then
      self._cursor_obj = _radiant.client.set_cursor(self._cursor)
   end

   stonehearth.selection:register_tool(self, true)

   self._xz_selector = _radiant.client.select_xz_region(self._selection_flags)
      :progress(function (box)
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
                                             :set_position(box.min:to_float())
            end
            if self._progress_cb then
               self._progress_cb(self, box)
            end
         end)
      :done(function (box)
            if self._done_cb then
               self._done_cb(self, box)
            end
         end)
      :fail(function()
            if self._fail_cb then
               self._fail_cb(self)
            end
         end)   
      :always(function()
            if self._always_cb then
               self._always_cb(self)
            end
         end)   
end

return XZRegionSelector