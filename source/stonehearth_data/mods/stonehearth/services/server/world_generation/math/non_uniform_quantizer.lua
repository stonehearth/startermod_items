local MathFns = require 'services.server.world_generation.math.math_fns'

local NonUniformQuantizer = class()

local Interval = class()

function Interval:__init(lower, upper)
   self.lower = lower
   self.upper = upper
end

-- centroids must be an array of integers
function NonUniformQuantizer:__init(centroids)
   table.sort(centroids)

   self._count = #centroids
   self._min = centroids[1]
   self._max = centroids[self._count]

   local interval_map = {}
   local lower, upper, interval

   for i=1, self._count-1 do
      lower = centroids[i]
      upper = centroids[i+1]
      -- use a lua type instead of a C type so luajit can extremely agressively optimze it.
      interval = Interval(lower, upper)

      for x=lower, upper-1 do
         interval_map[x] = interval
      end
   end

   interval_map[upper] = interval

   self._interval_map = interval_map
end

function NonUniformQuantizer:quantize(value)
   if value <= self._min then
      return self._min
   end

   if value >= self._max then
      return self._max
   end

   local key, interval, lower, upper, lower_delta, upper_delta

   key = MathFns.round(value)
   interval = self._interval_map[key]
   lower = interval.lower
   upper = interval.upper

   lower_delta = value - lower
   upper_delta = upper - value

   -- round midpoint to upper value
   if lower_delta < upper_delta then
      return lower
   else
      return upper
   end
end

return NonUniformQuantizer
