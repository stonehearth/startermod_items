local SlewRateFilter = class()

function SlewRateFilter:__init(slew_rate)
   self.slew_rate = slew_rate
end

function SlewRateFilter:filter(dst, src, src_len, sampling_interval)
   if sampling_interval ~= nil then assert(sampling_interval == 1) end

   local slew_rate = self.slew_rate
   local i, curr, prev

   prev = src[1]

   for i=1, src_len do
      curr = src[i]
      if curr - prev > slew_rate then
         curr = prev + slew_rate
      elseif prev - curr > slew_rate then
         curr = prev - slew_rate
      end
      dst[i] = curr
      prev = curr
   end
end

return SlewRateFilter
