local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Region3 = _radiant.csg.Region3

local FloorEraser = class()

-- this is the component which manages the fabricator entity.
function FloorEraser:__init(build_service)
   self._build_service = build_service
   self._log = radiant.log.create_logger('builder')
end

function FloorEraser:go(response)
   stonehearth.selection:select_xz_region()
      :set_cursor('stonehearth:cursors:create_floor')
      :use_manual_marquee(function(selector, box)
            return _radiant.client.create_voxel_node(1, Region3(box), 'materials/blueprint.material.xml', Point3f(0.5, 0, 0.5))
         end)
      :done(function(selector, box)
            self:_erase_floor(response, selector, box)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')            
         end)
      :go()

   return self
end

function FloorEraser:_erase_floor(response, selector, box)
   _radiant.call_obj(self._build_service, 'erase_floor_command', box)
      :done(function(r)
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
      :always(function()
            selector:destroy()
         end)
end

function FloorEraser:destroy()
end

return FloorEraser 