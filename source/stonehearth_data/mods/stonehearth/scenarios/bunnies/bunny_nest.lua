local BunnyNest = class()

function BunnyNest:__init()
end

function BunnyNest:initialize(properties, services)
   local rabbit_name = 'stonehearth:rabbit'
   local rng = services.rng
   local width = properties.size.width
   local length = properties.size.length
   local offset = width/4
   local increment = offset*2
   local dx, dy

   -- place_entity is valid from (1, 1) to (width, length) inclusive
   services:place_entity(rabbit_name, math.floor(width/2), math.floor(length/2))

   for j=offset, length, increment do
      for i=offset, length, increment do
         dx = rng:get_int(0, offset-1)
         dy = rng:get_int(0, offset-1)
         services:place_entity(rabbit_name, i+dx, j+dy)
      end
   end
end

return BunnyNest
