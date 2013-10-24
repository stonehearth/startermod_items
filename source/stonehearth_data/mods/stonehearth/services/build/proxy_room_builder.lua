local Constants = require 'services.build.constants'
local ProxyColumn = require 'services.build.proxy_column'
local ProxyWall = require 'services.build.proxy_wall'
local ProxyBuilder = require 'services.build.proxy_builder'
local ProxyRoomBuilder = class(ProxyBuilder)

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyRoomBuilder:__init()
   self[ProxyBuilder]:__init(self, self._on_mouse_event, self._on_keyboard_event)
end

function ProxyRoomBuilder:go()
   self:_start()   
   
   self._top_left  = self:add_column():move_to(Point3(-3, 0, -3))
   self._top_right = self:add_column():move_to(Point3(-3, 0, 3))
   self._bot_left  = self:add_column():move_to(Point3(3, 0, -3))
   self._bot_right = self:add_column():move_to(Point3(3, 0, 3))
   
   self._top = self:add_wall()
   self._bot = self:add_wall()
   self._left = self:add_wall()
   self._right = self:add_wall()

   self._door = self:add_door()
   self._bot:add_child(self._door)
   
   self:layout()
   
   return self
end

function ProxyRoomBuilder:layout()
   -- make sure we draw walls in counter-clockwise order so the
   -- normals face outward
   self._bot:connect_to(self._bot_left, self._bot_right)
   self._right:connect_to(self._bot_right, self._top_right)
   self._top:connect_to(self._top_right, self._top_left)
   self._left:connect_to(self._top_left, self._bot_left)
   
   local length = self._bot:get_length()
   local position = self._bot:reface_point(Point3(length / 2, 0, 0))
   self._door:move_to(position)
   self._bot:layout()
end

function ProxyRoomBuilder:_on_mouse_event(e)
   local query = _radiant.client.query_scene(e.x, e.y)
   if query.location then
      local location = query.location + query.normal
      self:move_to(location)
      if e:up(1) then
         self:publish()
         return false
      elseif e:up(2) then
         self:rotate()
      end
      return true
   end
   return false
end

function ProxyRoomBuilder:_on_keyboard_event(e)
   return false
end

return ProxyRoomBuilder
