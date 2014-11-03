local build_util = require 'lib.build_util'
local constants = require('constants').construction

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
   self._cut_region = _radiant.client.alloc_region3()
   _radiant.renderer.add_terrain_cut(self._cut_region)
end

function FloorEditor:go(response, brush_shape)
   local brush = _radiant.voxel.create_brush(brush_shape)

   log:detail('running')
   stonehearth.selection:select_xz_region()
      :require_unblocked(false)
      :select_front_brick(false)
      :allow_select_cursor(true)
      :set_cursor('stonehearth:cursors:create_floor')
      :set_find_support_filter(function(result)

            -- stop if we've hit a piece of existing floor.  the call to  :allow_select_cursor(true)
            -- will save us most, but not all, of the time.  without both checks, we will occasionally
            -- stab through existing floor blueprints and hit the bottom of the terrain cut, creating
            -- another slab of floor below this one.
            --
            local entity = result.entity
            if entity then               
               local fc = entity:get_component('stonehearth:fabricator')
               if fc then
                  local blueprint = build_util.get_blueprint_for(entity)
                  if blueprint:get_component('stonehearth:floor') then
                     return true
                  end
               end
            end
            
            -- defer to the common implementation
            return stonehearth.selection.find_supported_xz_region_filter(result)
         end)
      :use_manual_marquee(function(selector, box)
            local box_region = Region3(box)
            local model = brush:paint_through_stencil(box_region)
            local node =  _radiant.client.create_voxel_node(1, model, 'materials/blueprint.material.xml', Point3.zero)
            node:set_position(MODEL_OFFSET)

            -- Update the cut region for the floor
            self._cut_region:modify(function(cursor)
                  cursor:copy_region(box_region)
               end)
            return node
         end)
      :done(function(selector, box)
            log:detail('box selected')
            self:_add_floor(response, selector, box, brush_shape)
         end)
      :fail(function(selector)
            log:detail('failed to select box')
            response:reject('no region')
         end)
      :always(function()
            log:detail('selector called always')
            _radiant.renderer.remove_terrain_cut(self._cut_region)
            self._cut_region = nil
         end)
      :go()

   return self
end

function FloorEditor:_add_floor(response, selector, box, brush_shape)
   log:detail('calling server to create floor')

   _radiant.call_obj(self._build_service, 'add_floor_command', 'stonehearth:entities:wooden_floor', box, brush_shape)
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