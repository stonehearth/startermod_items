local BoundaryNormalizingFilter = class()
-- instead of extending signal at boundaries, pretend they don't exist and normalize filter coefficients
-- this preserves DC amplitude and prevents overweighting of certain elements

-- assumes coefficients are symmetric!
function BoundaryNormalizingFilter:__init(coefficients)
   local i
   local f = coefficients
   local f_len = #f
   local n = {}

   assert(f_len % 2 == 1) -- assert number of coefficients is odd
   assert(f[1] == f[f_len]) -- symmetry check, should check other values too

   local f_sum = 0
   for i=1, f_len, 1 do
      f_sum = f_sum + f[i]
   end

   -- calculate normalizaion coefficients for boundary conditions
   local boundary_len = (f_len-1)/2 -- number of elements with boundary effects
   local norm_sum = 0
   for i=1, boundary_len, 1 do
      norm_sum = norm_sum + f[i]
   end

   -- lower indicies of n are closer to the boundary (and sum the fewest components of f)
   for i=1, boundary_len, 1 do
      norm_sum = norm_sum + f[i+boundary_len]
      n[i] = f_sum / norm_sum
   end

   self._f = f
   self._f_len = f_len
   self._n = n
   self._boundary_len = boundary_len
   self._f_sum = f_sum -- for debugging
end

-- f is symmetric, so no need to reverse axis for convolution
function BoundaryNormalizingFilter:filter(dst, src, src_len, sampling_interval)
   if not sampling_interval then sampling_interval = 1 end

   -- using locals for readability and performance
   local f = self._f
   local f_len = self._f_len
   local n = self._n
   local boundary_len = self._boundary_len
   assert(src_len >= f_len)

   local i, j
   local k, k_start, k_end
   local offset = -boundary_len-1
   local sum

   j = 1
   for i=1, src_len, sampling_interval do
      sum = 0

      if i <= boundary_len then
         -- handle boundary conditions at start
         k_start = boundary_len+2 - i
         for k=k_start, f_len, 1 do
            sum = sum + src[i+offset+k]*f[k]
         end
         dst[j] = sum * n[i]
      else
         if i > src_len-boundary_len then
            -- handle boundary conditions at end
            k_end = f_len-boundary_len + src_len-i 
            for k=1, k_end, 1 do
               sum = sum + src[i+offset+k]*f[k]
            end
            dst[j] = sum * n[src_len-i+1]
         else
            -- main convolution loop
            for k=1, f_len, 1 do
               sum = sum + src[i+offset+k]*f[k]
            end
            dst[j] = sum
         end
      end

      j = j+1
   end

   assert((j-1) == self:calc_resampled_length(src_len, sampling_interval))
end

function BoundaryNormalizingFilter:calc_resampled_length(src_len, sampling_interval)
   return math.ceil(src_len / sampling_interval)
end

----------

local delta = 1e-14

function BoundaryNormalizingFilter._test()
   local f = {
      0.0825252748278993,
      0.1028667802618770,
      0.1189486784294360,
      0.1292563319035470,
      0.1328058691544810,
      0.1292563319035470,
      0.1189486784294360,
      0.1028667802618770,
      0.0825252748278993
   }

   BoundaryNormalizingFilter._test_impulse_response(f)
   BoundaryNormalizingFilter._test_boundary_normalization(f)
   BoundaryNormalizingFilter._test_resampling(f)
end

function BoundaryNormalizingFilter._test_impulse_response(f)
   local flt = BoundaryNormalizingFilter(f)
   local length = 40
   local src = {}
   local dst = {}
   local i, k

   for i=1, length, 1 do
      src[i] = 0
   end
   src[20] = 1

   flt:filter(dst, src, length)

   k = 1
   for i=20-flt._boundary_len, 20+flt._boundary_len, 1 do
      assert(math.abs(f[k]-dst[i]) < delta)
      k = k + 1
   end
end

-- confirm normalized DC boundary value
function BoundaryNormalizingFilter._test_boundary_normalization(f)
   local flt = BoundaryNormalizingFilter(f)
   local length = 40
   local src = {}
   local dst = {}
   local i, k

   for i=1, length, 1 do
      src[i] = 1
   end

   flt:filter(dst, src, length)

   for i=1, length, 1 do
      assert(math.abs(flt._f_sum-dst[i]) < delta)
   end
end

function BoundaryNormalizingFilter._test_resampling(f)
   local flt = BoundaryNormalizingFilter(f)
   local length = 40
   local src = {}
   local dst = {}
   local i, k

   for i=1, length, 1 do
      src[i] = 1
   end

   flt:filter(dst, src, length, 4)

   for i=1, length/4, 1 do
      assert(math.abs(flt._f_sum-dst[i]) < delta)
   end
end

return BoundaryNormalizingFilter
