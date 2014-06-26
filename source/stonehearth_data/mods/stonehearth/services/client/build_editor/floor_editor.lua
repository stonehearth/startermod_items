local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Region3 = _radiant.csg.Region3

local MODEL_OFFSET = Point3f(-0.5, 0, -0.5)

local FloorEditor = class()

-- this is the component which manages the fabricator entity.
function FloorEditor:__init(build_service)
self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
end

function FloorEditor:go(response, brush_shape)
   local brush = _radiant.voxel.create_brush(brush_shape)

   stonehearth.selection:select_xz_region()
      :set_cursor('stonehearth:cursors:create_floor')
      :use_manual_marquee(function(selector, box)
            local model = brush:paint_through_stencil(Region3(box))
            local node =  _radiant.client.create_voxel_node(1, model, 'materials/blueprint.material.xml', Point3f.zero)
            node:set_position(MODEL_OFFSET)
            return node
         end)
      :done(function(selector, box)
            self:_add_floor(response, selector, box, brush_shape)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')            
         end)
      :go()

   return self
end

function FloorEditor:_add_floor(response, selector, box, brush_shape)
   _radiant.call_obj(self._build_service, 'add_floor_command', 'stonehearth:entities:wooden_floor', box, brush_shape)
      :done(function(r)
            if r.new_selection then
               stonehearth.selection:select_entity(r.new_selection)
            end
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
      :always(function()
            selector:destroy()
         end)
end

function FloorEditor:destroy()
end

return FloorEditor 