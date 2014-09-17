local ZoneRenderer = require 'renderers.zone_renderer'
local Region2 = _radiant.csg.Region2
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local Color4 = _radiant.csg.Color4

local NoConstructionZoneRenderer = class()

local OVERLAPPING = {
   stripes = Color4(255, 0, 0, 255),
   border  = Color4(255, 0, 0, 255),
}
local NORMAL = {
   stripes = Color4(128, 128, 128, 255),
   border  = Color4( 32,  32,  32, 255),
}

function NoConstructionZoneRenderer:initialize(render_entity, datastore)
   self._zone_renderer = ZoneRenderer(render_entity)   
   
   -- trace the datastore so we can change the color whenever the overlap
   -- changes
   self._datastore = datastore
   self._datastore_trace = datastore:trace('rendering no construction zone')
                              :on_changed(function()
                                    self:_update_shape()
                                 end)

   -- trace the region so we can change the shape of the zone whenever the
   -- envelope changes                                 
   local entity = render_entity:get_entity()
   local rcs = entity:get_component('region_collision_shape')
   self._region = rcs:get_region()

   self._region_trace = rcs:trace_region('rendering no construction zone')
                           :on_changed(function()
                                 self:_update_shape()
                              end)
                              
   self:_update_shape()                              
end

function NoConstructionZoneRenderer:destroy()
   if self._region_trace then
      self._region_trace:destroy()
      self._region_trace = nil
   end
   if self._region_trace then
      self._region_trace:destroy()
      self._region_trace = nil
   end
   if self._datastore_trace then
      self._datastore_trace:destroy()
      self._datastore_trace = nil
   end
   if self._zone_renderer then
      self._zone_renderer:destroy()
      self._zone_renderer = nil
   end
end

function NoConstructionZoneRenderer:_update_shape()
   -- flatten the envelop for the reserved space for construction into the
   -- footprint so we can create a designation at the base of the building
   local region2 = Region2()
   for cube in self._region:get():each_cube() do
      region2:add_cube(Rect2(Point2(cube.min.x, cube.min.z),
                             Point2(cube.max.x, cube.max.z)))
   end

   -- figure out what color to draw the zone.
   local empty = next(self._datastore:get_data().overlapping) == nil
   local colors = empty and NORMAL or OVERLAPPING

   -- recreate the designation
   self._zone_renderer:set_designation_colors(colors.border, colors.stripes)
   self._zone_renderer:set_region2(region2)
end

return NoConstructionZoneRenderer
