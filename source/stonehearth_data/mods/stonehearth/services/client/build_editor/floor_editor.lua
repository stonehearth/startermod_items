local build_util = require 'lib.build_util'
local constants = require('constants').construction
local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local MODEL_OFFSET = Point3(-0.5, 0, -0.5)

local FloorEditor = class()
local log = radiant.log.create_logger('build_editor.floor')

-- this is the component which manages the fabricator entity.
function FloorEditor:__init(build_service)
   log:debug('created')

   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
end

function FloorEditor:go(response, floor_uri, options)
   local brush_shape = voxel_brush_util.brush_from_uri(floor_uri)
   log:detail('running')

   local brush = _radiant.voxel.create_brush(brush_shape)
   local selector = stonehearth.selection:select_xz_region()
   
   if options.sink_floor then
      -- create a terrain cut region to remove the terrain overlapping the cursor
      self._cut_region = _radiant.client.alloc_region3()
      _radiant.renderer.add_terrain_cut(self._cut_region)

      -- the default xz region selector will grab the blocks adjacent to the one
      -- the cursor is under.  we want to sink into the terrain, so return the
      -- actual brink we're pointing to instead
      selector:select_front_brick(false)
      selector:set_validation_offset(Point3.unit_y)

      -- we need to be able to intersect our cursor in the query function to make
      -- sure we get points on a plane.  if we're not sinking, though, we want to
      -- pierce the cursor to make sure we hit the actual ground.
      selector:allow_select_cursor(true)

      selector:require_unblocked(false)
   else
      -- at least when placing slabs, make sure there's empty space when dragging
      -- the cursor
      selector:require_unblocked(true)
   end

   selector
      :set_cursor('stonehearth:cursors:create_floor')
      :set_find_support_filter(stonehearth.selection.make_edit_floor_xz_region_filter())
      :set_can_contain_entity_filter(stonehearth.selection.floor_can_contain)
      :use_manual_marquee(function(selector, box)
            local box_region = Region3(box)
            local model = brush:paint_through_stencil(box_region)
            local node =  _radiant.client.create_voxel_node(1, model, 'materials/blueprint.material.xml', Point3.zero)
            node:set_position(MODEL_OFFSET)
            node:set_polygon_offset(-5, -5)

            if self._cut_region then
               -- Update the cut region for the floor
               self._cut_region:modify(function(cursor)
                     cursor:copy_region(box_region)
                  end)
            end

            return node
         end)
      :done(function(selector, box)
            log:detail('box selected')
            self:_add_floor(response, selector, box, floor_uri)
         end)
      :fail(function(selector)
            log:detail('failed to select box')
            if self._cut_region then
               _radiant.renderer.remove_terrain_cut(self._cut_region)
               self._cut_region = nil
            end
            response:reject('no region')
         end)
      :always(function()
            log:detail('selector called always')
         end)
      :go()

   return self
end

function FloorEditor:_add_floor(response, selector, box, floor_uri)
   log:detail('calling server to create floor')

   _radiant.call_obj(self._build_service, 'add_floor_command', floor_uri, box)
      :done(function(r)
            log:detail('server call to create floor finished')
            if r.new_selection then
               stonehearth.selection:select_entity(r.new_selection)
            end
            response:resolve(r)
         end)
      :fail(function(r)
            log:detail('server call to create floor failed')
            response:reject(r)
         end)
      :always(function()
            if self._cut_region then
               _radiant.renderer.remove_terrain_cut(self._cut_region)
               self._cut_region = nil
            end
         end)
end

return FloorEditor 