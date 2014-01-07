local ARGS = {}
local PREV = {}

CALL_PLACEHOLDER_mt = {}
function CALL_PLACEHOLDER_mt.__call(t, compound_action)
   if t.type == ARGS then
      return compound_action:_get_arg(t.key)
   elseif t.type == PREV then
      return compound_action:_get_previous_run_arg(1, t.key)
   end
   assert(false, string.format('unknown placeholder type'))
end

PLACEHOLDER_mt = {}
function PLACEHOLDER_mt.__index(t, key)
   local result = {
      __placeholder = true,
      type = t,
      key = key
   }
   setmetatable(result, CALL_PLACEHOLDER_mt)
   return result
end
setmetatable(ARGS, PLACEHOLDER_mt)
setmetatable(PREV, PLACEHOLDER_mt)

return {
   ARGS = ARGS,
   PREV = PREV,
}
