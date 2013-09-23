--[[
A leash is the movement pattern of a mob, whereby the mob will always stay
within 
]]
local Point3 = _radiant.csg.Point3
local Leash = class()


function Leash:__init(entity)
   self._entity = entity  -- the entity this component is attached to
   self._aggro_radius = 0
   self._leash_radius = 0
   self._location = Point3(0, 0, 0)
end

function Leash:extend(json)
   if json and json.aggro_radius then
      self._aggro_radius = json.aggro_radius
   end

   if json and json.leash_radius then
      self._leash_radius = json.leash_radius
   end

end

function Leash:get_location()
   return self._location
end

function Leash:set_location(point)
   self._location = point
end

return Leash