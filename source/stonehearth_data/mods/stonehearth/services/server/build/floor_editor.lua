local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Region3 = _radiant.csg.Region3

local FloorEditor = class()

-- this is the component which manages the fabricator entity.
function FloorEditor:__init()
   self._log = radiant.log.create_logger('builder')
end

function FloorEditor:go(response)
   stonehearth.selection.select_xz_region()
      :set_cursor('stonehearth:cursors:create_floor')
      :use_manual_marquee(function(selector, box)
            return _radiant.client.create_voxel_node(1, Region3(box), 'materials/blueprint.material.xml', Point3f(0.5, 0, 0.5))
         end)
      :done(function(selector, box)
            self:_add_floor(selector, box, response)
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')            
         end)
      :go()

   return self
end

function FloorEditor:_add_floor(selector, box, response)   
   _radiant.call('stonehearth:add_floor', 'stonehearth:entities:wooden_floor', box)
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