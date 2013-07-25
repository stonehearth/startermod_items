local util = {}

--[[
  Was failing whenever the object was a RadiantI3 because obj:is_valid() was undefined.
]]

function util.is_string(obj)
   return type(obj) == 'string'
end

function util.is_number(obj)
   return type(obj) == 'number'
end

function util.colorcode_to_integer(cc)
   local result = 0
   if type(cc) == 'string' then
      -- parse the digits out of 'rgb(13, 34, 19)', allowing for spaces between tokens
      local r, g, b = cc:match('rgb%s*%(%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*%)')
      if r and g and b then
         result = (((tonumber(b) * 256) + tonumber(g)) * 256) + tonumber(r)
      end
   end
   return result
end

return util
