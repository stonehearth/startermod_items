local HerdOfSheep = class()

function HerdOfSheep:__init()
end

function HerdOfSheep:initialize(context)
   local sheep_name = 'stonehearth:sheep'
   local place_entity = context.place_entity
   local rng = context.rng
   local width = context.scenario.size.width
   local length = context.scenario.size.length
   local offset = width/4
   local increment = offset*2
   local dx, dy

   -- place item is valid from (1, 1) to (width, length) inclusive
   place_entity(sheep_name, math.floor(width/2), math.floor(length/2))

   for j=offset, length, increment do
      for i=offset, length, increment do
         dx = rng:get_int(0, offset-1)
         dy = rng:get_int(0, offset-1)
         place_entity(sheep_name, i+dx, j+dy)
      end
   end
end

return HerdOfSheep
