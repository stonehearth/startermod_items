local HerdOfSheep = class()

function HerdOfSheep:__init()
end

function HerdOfSheep:initialize(properties, services)
   local sheep_name = 'stonehearth:sheep'
   local rng = services.rng
   local width = properties.size.width
   local length = properties.size.length
   local offset = width/4
   local increment = offset*2
   local margin = 2
   local child_density = 0.7
   local dx, dy

   -- place_entity is valid from (1, 1) to (width, length) inclusive
   services:place_entity(sheep_name, math.floor(width/2), math.floor(length/2))

   for j=offset, length, increment do
      for i=offset, length, increment do
         if rng:get_real(0, 1) < child_density then
            dx = rng:get_int(0, offset-margin)
            dy = rng:get_int(0, offset-margin)
            services:place_entity(sheep_name, i+dx, j+dy)
         end
      end
   end
end

return HerdOfSheep