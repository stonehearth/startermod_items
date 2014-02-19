local MathFns = require 'services.world_generation.math.math_fns'

local Histogram = class()

function Histogram:__init()
   self:clear()
end

function Histogram:clear()
   self._counts = {}
   self._total = 0
end

function Histogram:get_total()
   return self._total
end

function Histogram:increment(key, count)
   if count == nil then
      count = 1
   end

   local old_value = self._counts[key]

   if old_value == nil then
      self._counts[key] = count
   else
      self._counts[key] = old_value + count
   end

   self._total = self._total + count
end

function Histogram:get_count(key)
   local value = self._counts[key]

   if value == nil then
      return 0
   else
      return value
   end
end

function Histogram:get_probability(key)
   return self:get_count(key) / self._total
end

function Histogram:print(logger, order, format_string)
   if order == nil then
      for key, _ in pairs(self._counts) do
         self:_print_key(logger, key, format_string)
      end
   else
      for _, key in ipairs(order) do
         self:_print_key(logger, key, format_string)
      end
   end
end

function Histogram:_print_key(logger, key, format_string)
   if format_string == nil then format_string = '%.0f' end

   local probability = self:get_probability(key)
   logger:debug('%s: %s%%', key, string.format(format_string, MathFns.round(probability*100)))
end

return Histogram
