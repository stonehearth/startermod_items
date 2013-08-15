local Daubechies_D4 = {}

-- Daubechies D4 wavelet coefficients
local H1 =  0.482962913144534
local H2 =  0.836516303737808
local H3 =  0.224143868042013
local H4 = -0.129409522551260
   
local G1 = -0.129409522551260
local G2 = -0.224143868042013
local G3 =  0.836516303737808
local G4 = -0.482962913144534

-- false creates leakage that accumulates on boundary
local perfect_reconstruction = true

function Daubechies_D4.DWT_1D(src, length)
   local i, n, A, B, C, D
   local temp = {}

   n = length/2

   assert(length % 2 == 0)
   assert(length >= 4)

   for i=1, n, 1 do
      A = src[2*i-1]
      B = src[2*i]
      
      if i < n then
         C = src[2*i+1]
         D = src[2*i+2]
      else
         if perfect_reconstruction then
            C = src[1] -- periodic extension at boundary for perfect reconstruction
            D = src[2]
         else
            C = B -- flat extension at boundary to minimize artifacts
            D = B
         end
      end

      temp[i]   = A*H4 + B*H3 + C*H2 + D*H1 -- low frequency
      temp[n+i] = A*G4 + B*G3 + C*G2 + D*G1 -- high frequency
   end

   for i=1, length, 1 do
      src[i] = temp[i]
   end
end

function Daubechies_D4.IDWT_1D(src, length)
   local i, n, A, B, C, D
   local temp = {}

   n = length/2

   assert(length % 2 == 0)
   assert(length >= 4)

   for i=1, n, 1 do
      B = src[i]
      D = src[n+i]

      if i ~= 1 then
         A = src[i-1]
         C = src[n+i-1]
      else
         if perfect_reconstruction then
            A = src[n] -- inverse of the periodic extension
            C = src[n+n]
         else
            A = B -- copy low freq
            C = 0 -- zero high freq
         end
      end

      temp[2*i-1] = A*H2 + B*H4 + C*G2 + D*G4
      temp[2*i]   = A*H1 + B*H3 + C*G1 + D*G3
   end

   for i=1, length, 1 do
      src[i] = temp[i]
   end
end

return Daubechies_D4

