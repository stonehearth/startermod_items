local mods = {}

function mods.__init()
end

function mods.require(s)
   return _host:require(s)
end

function mods.load_script(s)
   return _host:require_script(s)
end

function mods.enum_objects(path)
   -- The stack offset for the helper functions is 3...
   --    1: __get_current_module_name
   --    2: mods.enum_objects
   --    3: --> some module whose name we want! <-- 
   local modname = __get_current_module_name(3)
   return _host:enum_objects(modname, path)
end

function mods.read_object(path)
   local modname = __get_current_module_name(3)
   return _host:read_object(modname, path)
end

function mods.write_object(path, obj)
   local modname = __get_current_module_name(3)
   return _host:write_object(modname, path, obj)
end

return mods
