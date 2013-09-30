--[[
A LeashComponent is the movement pattern of a mob, whereby the mob will always stay
within 
]]
local Point3 = _radiant.csg.Point3
local LeashComponent = class()


function LeashComponent:__init(entity)
   self._entity = entity  -- the entity this component is attached to
   self._aggro_radius = 0
   self._LeashComponent_radius = 0
   self._location = Point3(0, 0, 0)
end

function LeashComponent:extend(json)
   if json and json.aggro_radius then
      self._aggro_radius = json.aggro_radius
   end

   if json and json.LeashComponent_radius then
      self._LeashComponent_radius = json.LeashComponent_radius
   end

end

function LeashComponent:get_location()
   return self._location
end

function LeashComponent:set_location(point)
   self._location = point
end

function LeashComponent:get_LeashComponent_radius()
   return self._LeashComponent_radius
end

function LeashComponent:set_LeashComponent_radius(radius)
   self._LeashComponent_radius = radius
end
return LeashComponent