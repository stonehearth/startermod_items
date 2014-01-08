
PLACEHOLDER_mt = {}
function PLACEHOLDER_mt.__call(t, ...)
   local call_args = {...}
   local ftab = {
      eval = function (p, compound_action)         
         local obj = t:__eval(compound_action)
         local args = {}
         for _, arg in ipairs(call_args) do
            if type(arg) == 'table' and arg.__placeholder then
               arg = arg:__eval(compound_action)
            end
            table.insert(args, arg)
         end         
         return obj(unpack(args))
      end,
      tostring = function ()
         local args = ''
         for _, arg in ipairs(call_args) do
            if #msg > 0 then
               msg = msg .. ', '
            end
            msg = msg .. radiant.util.tostring(arg)
         end      
         return tostring(t) .. '(' .. args .. ')'
      end
   }
   return create_placeholder(ftab)
end

function PLACEHOLDER_mt.__index(t, key)
   local ftab = {
      eval = function (p, compound_action)         
         local obj = t:__eval(compound_action)
         return obj[key]
      end,
      tostring = function ()
         return tostring(t) .. '.' .. key
      end
   }
   return create_placeholder(ftab)
end

function PLACEHOLDER_mt.__tostring(t)
   return t.__placeholder.tostring(t)
end

function create_placeholder(ftab)
   local result = {   
      __eval = ftab.eval,
      __placeholder = ftab
   }
   setmetatable(result, PLACEHOLDER_mt)
   return result
end

local ARGS = {
   eval = function (p, compound_action)
      return compound_action:_get_args()
   end,
   tostring = function()
      return 'ARGS'
   end
}

local PREV = {
   eval = function (p, compound_action)
      return compound_action:_get_previous_think_output(1)
   end,
   tostring = function()
      return 'PREV'
   end
}

local BACK = {
   eval = function (p, compound_action)
      return function (i)
         return compound_action:_get_previous_think_output(i)
      end
   end,
   tostring = function()
      return 'BACK'
   end
}

local CURRENT = {
   eval = function (p, compound_action)
      return compound_action:_get_current_entity_state()
   end,
   tostring = function()
      return 'CURRENT'
   end
}

return {
   ARGS = create_placeholder(ARGS),
   PREV = create_placeholder(PREV),
   BACK = create_placeholder(BACK),
   CURRENT = create_placeholder(CURRENT),
}
