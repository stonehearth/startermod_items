
function radiant.keys(t)
   local n, keys = 1, {}
   for k, _ in pairs(t) do
      keys[n] = k
      n = n + 1
   end
   return keys
end
