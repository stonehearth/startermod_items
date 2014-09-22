local mods = {}

function mods.__init()
end

function mods.require(s)
   return _host:require(s)
end

function mods.load_script(s)
   return _host:require_script(s)
end

function mods.create_object(arg1, arg2)
   local ctor = mods.require(arg1, arg2)
   return ctor()
end

function mods.get_all_entities()
   local mods = radiant.resources.get_mod_list()

   for i, mod in ipairs(mods) do
      local manifest = radiant.resources.load_manifest(mod)
      if manifest.aliases then
         for alias, path in manifest.aliases do
            --rehydrate the mod
            mods._all_entities[alias] = alias
         end
      end
   end
end

mods.__init()
return mods
