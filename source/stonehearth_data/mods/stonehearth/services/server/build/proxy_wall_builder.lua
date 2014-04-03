local constants = require('constants').construction
local ProxyColumn = require 'services.server.build.proxy_column'
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyBuilder = require 'services.server.build.proxy_builder'
local ProxyWallBuilder = class(ProxyBuilder)

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyWallBuilder:__init()
   self[ProxyBuilder]:__init(self, self._on_mouse_event, self._on_keyboard_event)
end

function ProxyWallBuilder:go(response)
   self:_start()
   self:add_column()
   self._response = response
   
   return self
end

function ProxyWallBuilder:_get_last_segment()
   return self:get_column(-2), self:get_column(-1)
end

function ProxyWallBuilder:_fit_point_to_constraints(absolute_pt)
   local column_a, column_b = self:_get_last_segment()
   if not column_a then
      return absolute_pt
   end
   -- constrain the 2nd column to be no more than the max wall
   -- distance away from the first
   local pt_a = column_a:get_location_absolute()
   local pt_b = absolute_pt

   local t, n, dt, d
   if math.abs(pt_a.x - pt_b.x) >  math.abs(pt_a.z - pt_b.z) then
      t = 'x'
      n = 'z'
   else
      t = 'z'
      n = 'x'
   end
   if pt_a[t] > pt_b[t] then
      dt = 1
      d  = math.max(pt_b[t] - pt_a[t], -constants.MAX_WALL_SPAN)
   else
      dt = -1
      d  = math.min(pt_b[t] - pt_a[t], constants.MAX_WALL_SPAN)
   end
   pt_b[t] = pt_a[t] + d
   pt_b[n] = pt_a[n]
   pt_b.y = pt_a.y
   
   local region = column_b:get_region():get()
   while not _radiant.client.is_valid_standing_region(region:translated(pt_b)) and pt_b[t] ~= pt_a[t] do
      pt_b[t] = pt_b[t] - dt
   end
   return pt_b
end

function ProxyWallBuilder:_add_new_segment()
   local column_a, column_b = self:_get_last_segment()
   if column_a and column_b then
      local wall = self:add_wall()
      wall:connect_to(column_a, column_b)
      return wall
   end
end

function ProxyWallBuilder:_on_mouse_event(e)
   local query = _radiant.client.query_scene(e.x, e.y)
   if query:is_valid() then
      local world_location = self:_fit_point_to_constraints(query:brick_of(0) + query:normal_of(0):to_int())
      if self:get_column_count() == 1 then
         -- if this is the very first column, move the room around.
         self:move_to(world_location)
      else 
         -- otherwise, move the column
         self:get_column(-1):move_to_absolute(world_location)
      end
      if e:up(1) then
         self:_add_new_segment()      
         if self:shift_down() or self:get_column_count() < 2 then
            self:add_column():move_to_absolute(world_location)
         else
            self:publish()
            self._response:resolve({ result = true })
         end
         return true
      end
   end
   return true
end

function ProxyWallBuilder:_on_keyboard_event(e)
   if e.key == _radiant.client.KeyboardInput.KEY_ESC and e.down then
      self:cancel()
   end
   return false
end

return ProxyWallBuilder
