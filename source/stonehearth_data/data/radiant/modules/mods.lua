local mods = {}

function mods.__init()
end

function mods.require(s)
   return _host:require_script(s)
end

function mods.get_singleton(mod_name)
   local m = radiant.resources.load_manifest(mod_name)
   if m and m.radiant and m.radiant.game_objects then
      local s = m.radiant.game_objects.singleton
      if s then
         return _host:require_script(s)
      end
   end
end

function mods.create_object(arg1, arg2)
   local ctor = mods.require(arg1, arg2)
   return ctor()
end

mods.__init()
return mods
