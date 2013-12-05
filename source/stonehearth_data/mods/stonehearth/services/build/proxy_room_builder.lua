local constants = require('constants').construction
local ProxyColumn = require 'services.build.proxy_column'
local ProxyWall = require 'services.build.proxy_wall'
local ProxyBuilder = require 'services.build.proxy_builder'
local ProxyRoomBuilder = class(ProxyBuilder)

local INFINITE = 10000000
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local Region2 = _radiant.csg.Region2
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2

-- this is the component which manages the fabricator entity.
function ProxyRoomBuilder:__init()
   self[ProxyBuilder]:__init(self, self._on_mouse_event, self._on_keyboard_event)
   self._curr_door_wall = nil
end

function ProxyRoomBuilder:go()
   self:_start()   
   
   self._bot_left  = self:add_column():move_to(Point3(-3, 0, -3))
   self._bot_right = self:add_column():move_to(Point3( 3, 0, -3))
   self._top_left  = self:add_column():move_to(Point3(-3, 0,  3))
   self._top_right = self:add_column():move_to(Point3( 3, 0,  3))
   
   self._top = self:add_wall()
   self._bot = self:add_wall()
   self._left = self:add_wall()
   self._right = self:add_wall()

   self._door = self:add_door()
   self._curr_door_wall = self._bot
   self._curr_door_wall:add_child(self._door)
   
   self._roof = self:add_roof()
   
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
     
   local length = self._curr_door_wall:get_length()
   local position = self._curr_door_wall:reface_point(Point3(length / 2, 0, 0))
   self._door:move_to(position)
   self._curr_door_wall:layout()
   
   local min = self._bot_left:get_location()
   local max = self._top_right:get_location()
   
   
   local roof_min = Point2(min.x - 1, min.z - 1)
   -- add 1 to reach the edge of the column and another to reach
   -- beyond.  this will break once columns get bigger than 1x1
   local roof_max = Point2(max.x + 2, max.z + 2)
   
   --[[
   local roof_min = Point2(0, 0)
   local roof_max = Point2(10, 10)
   ]]
   
   -- xxx: how do we say the offset a roof should use intead of this hardcoded -1?
   self._roof:move_to(Point3(0, constants.STOREY_HEIGHT - 1, 0))
   self._roof:cover_region(Region2(Rect2(roof_min, roof_max)))   
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
         -- regions cannot be rotated!
         -- self:rotate()
         -- The least we can do is rotate what wall the door appears on
         self:_move_door()
      end
      return true
   end
   return false
end

--Temporary, till we figure out how to rotate regions
function ProxyRoomBuilder:_move_door()
   --remove the door from the current wall
   self._curr_door_wall:remove_child(self._door)
   self._curr_door_wall:layout()

   --Change the wall (counter-clockwise)
   if self._curr_door_wall == self._bot then
      self._curr_door_wall = self._left
   elseif self._curr_door_wall == self._left then
      self._curr_door_wall = self._top 
   elseif self._curr_door_wall == self._top then
      self._curr_door_wall = self._right
   elseif self._curr_door_wall == self._right then
      self._curr_door_wall = self._bot
   end

   self._curr_door_wall:add_child(self._door)
   local length = self._curr_door_wall:get_length()
   local position = self._curr_door_wall:reface_point(Point3(length / 2, 0, 0))
   self._door:move_to(position)
   self._curr_door_wall:layout()
end

function ProxyRoomBuilder:_on_keyboard_event(e)
   return false
end

return ProxyRoomBuilder
