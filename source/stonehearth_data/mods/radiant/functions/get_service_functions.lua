local GetServiceFunctions = class()

local function split(str, sep)
        local fields = {}
        local pattern = string.format("([^%s]+)", sep)
        str:gsub(pattern, function(c) fields[#fields+1] = c end)
        return fields
end

local function rawget_dotted(str)
   local t = _G
   for _, var in pairs(split(str, '.')) do
      t = rawget(t, var)
      if type(t) ~= 'table' then
         return nil
      end
   end
   return t
end

local function get_service(session, request, name)
   local service = rawget_dotted(name)
   if service and service.__saved_variables then
      return service.__saved_variables
   end
   request:reject('no such service')
end

function GetServiceFunctions:get_service(session, request, name)
   return get_service(session, request, name)
end

function GetServiceFunctions:get_client_service(session, request, name)
   return get_service(session, request, name)
end

return GetServiceFunctions