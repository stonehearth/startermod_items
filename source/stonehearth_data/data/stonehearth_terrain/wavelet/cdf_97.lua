local CDF_97 = class()
-- Cohen-Daubechies-Feauveau 9/7 wavelet
-- excellent cutoff and orthogonality properties
-- lifting implementation so we can use symmetric boundary extension
-- n < 8 will have increasing boundary condition effects

local coeff = { -1.586134342, -0.05298011854, 0.8829110762, 0.4435068522 }
local scale = 1/1.149604398

local temp = {}

function CDF_97:__init()

end

function CDF_97:DWT_1D(src, length)
   assert(length % 2 == 0)
   assert(length >= 4)

   self:_predict(src, length, coeff[1]) -- Predict 1
   self:_update(src, length, coeff[2])  -- Update 1
   self:_predict(src, length, coeff[3]) -- Predict 2
   self:_update(src, length, coeff[4])  -- Update 2
   self:_scale(src, length, scale)      -- Scale
   self:_deinterleave(src, length)      -- consolidate frequncy blocks
end

function CDF_97:IDWT_1D(src, length)
   assert(length % 2 == 0)
   assert(length >= 4)

   self:_interleave(src, length)         -- interleave frequency components
   self:_scale(src, length, 1/scale)     -- Undo Scale
   self:_update(src, length, -coeff[4])  -- Undo Update 2
   self:_predict(src, length, -coeff[3]) -- Predict 2
   self:_update(src, length, -coeff[2])  -- Update 1
   self:_predict(src, length, -coeff[1]) -- Predict 1
end

function CDF_97:_predict(x, n, a)
   local i

   for i=2, n-2, 2 do
      x[i] = x[i] + a*(x[i-1]+x[i+1])
   end

   x[n] = x[n] + 2*a*x[n-1]
end

function CDF_97:_update(x, n, a)
   local i

   x[1] = x[1] + 2*a*x[2];

   for i=3, n, 2 do
      x[i] = x[i] + a*(x[i-1]+x[i+1])
   end
end

function CDF_97:_scale(x, n, a)
   local i
   local one_over_a = 1/a
   
   for i=1, n, 2 do
      x[i] = x[i] * one_over_a
      x[i+1] = x[i+1] * a
   end
end

function CDF_97:_deinterleave(x, n)
   local i
   local w = n*0.5

   for i=1, w, 1 do
      temp[i] = x[i*2-1]
      temp[i+w] = x[i*2]
   end

   for i=1, n, 1 do
      x[i] = temp[i]
   end
end

function CDF_97:_interleave(x, n)
   local i
   local w = n*0.5

   for i=1, w, 1 do
      temp[i*2-1] = x[i]
      temp[i*2] = x[i+w]
   end

   for i=1, n, 1 do
      x[i] = temp[i]
   end
end

----------

function CDF_97:_test_perfect_reconstruction()
   local i, j
   local length = 32
   local x = {}
   local y = {}
   local DELTA = 1e-13

   -- Makes a fancy cubic signal
   for i=1, length, 1 do
      j = i-1
      x[i] = 5 + j + 0.4*j^2 - 0.02*j^3
   end

   for i=1, length, 1 do
      y[i] = x[i]
   end

   CDF_97:DWT_1D(y, length)
   CDF_97:IDWT_1D(y, length)

   local diff
   for i=1, length, 1 do
      diff = math.abs(y[i]-x[i])
      assert(diff < DELTA)
   end
end

return CDF_97

