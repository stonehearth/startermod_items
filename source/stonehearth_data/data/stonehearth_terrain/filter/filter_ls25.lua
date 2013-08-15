-- 4th order least squares with a cutoff of 0.25 - 0.30

local F1 = 0.155116694247352
local F2 = 0.242799620041932
local F3 = 0.277195551932604

local F_sum = F1+F2+F3+F2+F1
local norm2 = F_sum / (F3+F2+F1)
local norm1 = F_sum / (F2+F3+F2+F1)

function filter_ls25(src, length)
   local i
   local temp = {}

   assert(length >= 4)

   i = 1
   temp[i] = (F3*src[i] + F2*src[i+1] + F1*src[i+2]) * norm2

   i = 2
   temp[i] = (F2*src[i-1] + F3*src[i] + F2*src[i+1] + F1*src[i+2]) * norm1

   for i=3, length-2, 1 do
      temp[i] = F1*src[i-2] + F2*src[i-1] + F3*src[i] + F2*src[i+1] + F1*src[i+2]
   end

   i = length-1
   temp[i] = (F1*src[i-2] + F2*src[i-1] + F3*src[i] + F2*src[i+1]) * norm1

   i = length
   temp[i] = (F1*src[i-2] + F2*src[i-1] + F3*src[i]) * norm2

   for i=1, length, 1 do
      src[i] = temp[i]
   end
end

return filter_ls25
