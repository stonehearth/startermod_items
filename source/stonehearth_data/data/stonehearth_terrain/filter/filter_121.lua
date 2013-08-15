
function filter_121(src, length)
   local i
   local temp = {}

   assert(length >= 2)

   i = 1
   temp[i] = (2*src[i] + src[i+1]) * 0.33333333333333333

   for i=2, length-1, 1 do
      temp[i] = (src[i-1] + 2*src[i] + src[i+1]) * 0.25
   end

   i = length
   temp[i] = (src[i-1] + 2*src[i]) * 0.33333333333333333


   for i=1, length, 1 do
      src[i] = temp[i]
   end
end

return filter_121
