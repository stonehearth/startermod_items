-- move to the client! -- tony
local ProxyColumn = require 'services.server.build.proxy_column'
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyPortal = require 'services.server.build.proxy_portal'
local ProxyRoof = require 'services.server.build.proxy_roof'
local ProxyContainer = require 'services.server.build.proxy_container'
local ProxyBuilder = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyBuilder:__init(derived, on_mouse, on_keyboard)
   self._derived = derived
   self._on_mouse = on_mouse
   self._on_keyboard = on_keyboard
   self._brushes = {
      wall = 'stonehearth:wooden_wall',
      column = 'stonehearth:wooden_column',
      door = 'stonehearth:wooden_door',
      roof = 'stonehearth:wooden_peaked_roof',
   }
   
   self._rotation = 0
   self._root_proxy = ProxyContainer(nil, 'stonehearth:entities:building')
                        :set_building(self._root_proxy)

   -- local entity = self._root_proxy:get_entity()
   -- entity:add_component('stonehearth:no_construction_zone')
end

function ProxyBuilder:set_brush(type, uri)
   self._brushes[type] = uri
end

function ProxyBuilder:_clear()
   if self._input_capture then
      self._input_capture:destroy()
      self._input_capture = nil
   end
   self._root_proxy:destroy()
   self._root_proxy = nil
   self._columns = nil
   self._walls = nil
end

function ProxyBuilder:_start()
   assert(not self._columns)
   assert(not self._walls)
   self._columns = {}
   self._walls = {}
   
   self._input_capture = stonehearth.input:capture_input()
                           :on_mouse_event(function(e)
                                 return self._on_mouse(self._derived, e)
                              end)
                           :on_keyboard_event(function(e)
                                 return self._on_keyboard(self._derived, e)
                              end)

end

function ProxyBuilder:move_to(location)
   self._root_proxy:move_to(location)
end

function ProxyBuilder:turn_to(angle)
   self._root_proxy:turn_to(angle)
end

function ProxyBuilder:get_column(i)
   if i > 0 then
      return self._columns[i]
   end
   return self._columns[#self._columns + i + 1]
end

function ProxyBuilder:get_column_count()
   return #self._columns
end

function ProxyBuilder:add_column()
   local column = ProxyColumn(self._root_proxy, self._brushes.column)
                     :set_building(self._root_proxy)

   table.insert(self._columns, column)
   return column
end

function ProxyBuilder:add_wall()
   local wall = ProxyWall(self._root_proxy, self._brushes.wall)
                     :set_building(self._root_proxy)

   table.insert(self._walls, wall)
   return wall
end

function ProxyBuilder:add_door()
   return ProxyPortal(self._root_proxy, self._brushes.door)
            :set_building(self._root_proxy)

end

function ProxyBuilder:add_roof()
   return ProxyRoof(self._root_proxy, self._brushes.roof)
            :set_building(self._root_proxy)
end

function ProxyBuilder:shift_down()
  return _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_LEFT_SHIFT) or
         _radiant.client.is_key_down(_radiant.client.KeyboardInput.KEY_RIGHT_SHIFT)
end

function ProxyBuilder:publish()
   stonehearth.builder:create_building(self._root_proxy)
   self:_clear()
end

function ProxyBuilder:cancel()
   self:_clear()
end

function ProxyBuilder:rotate()
   self._rotation = self._rotation + 90
   if self._rotation >= 360 then
      self._rotation = 0
   end
   self:turn_to(self._rotation)
   return self
end

return ProxyBuilder
