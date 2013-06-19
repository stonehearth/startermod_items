require 'unclasslib'

local Util = class()

local NativeClasses = {
   [RadiantIPoint3] = false,
   [AnimationResource] = false,
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

function Util:is_entity(obj)
   local is_entity = obj and self:is_a(obj, Entity)
   is_entity = is_entity and obj:is_valid()
   return is_entity
end

function Util:is_string(obj)
   return type(obj) == 'string'
end

function Util:is_number(obj)
   return type(obj) == 'number'
end

function Util:is_a(obj, cls)
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


function Util:is_callable(obj)
   return not self:is_a(obj, Entity) and
          (type(obj) == 'function' or (type(obj) == 'table' and (type(obj.__index) == 'table' or obj.__call ~= nil)))
end

return Util()
