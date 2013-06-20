local util = {}

local NativeClasses = {
   [RadiantIPoint3] = false,
   [Animation] = false,
   [Entity] = 'Entity',
   [UnitInfo] = 'UnitInfo',
   [Mob] = 'Mob',
   [EntityContainer] = 'EntityContainer',
   [Wall] = 'Wall',
   [Fixture] = 'Fixture',
   [EffectList] = 'EffectList',
   [Sensor] = 'Sensor',
   [TargetTable] = 'TargetTable',
}
--[[
  Was failing whenever the object was a RadiantI3 because obj:is_valid() was undefined.
]]
function util.is_entity(obj)
   local is_entity = obj and util.is_a(obj, Entity)
   if obj.is_valid then
      is_entity = is_entity and obj:is_valid()
   else
      is_entity = false
   end
   return is_entity
end

function util.is_string(obj)
   return type(obj) == 'string'
end

function util.is_number(obj)
   return type(obj) == 'number'
end

function util.is_a(obj, cls)
   if NativeClasses[cls] ~= nil then
      if type(obj) == 'userdata' then
         if obj.get_class_name then
            -- xxx: this doesn't work with parent classes =(
            --return NativeClasses[cls] == obj:get_class_name();
            return true
         end
         -- xxx: need to enforce that everyone implements get_class_name... or better yet, native:is_a !!
         return true
      end
      return false
-- xxx : class_info(obj) is causing weird crashes in the backend when destroying the result!
      --return type(obj) == 'userdata' and class_info(obj).name == NativeClasses[cls] -- native object
   end
   return (classof(obj) and obj:is_a(cls))  -- lua classlib
end


function util.is_callable(obj)
   return not util.is_a(obj, Entity) and
          (type(obj) == 'function' or (type(obj) == 'table' and (type(obj.__index) == 'table' or obj.__call ~= nil)))
end

return util
