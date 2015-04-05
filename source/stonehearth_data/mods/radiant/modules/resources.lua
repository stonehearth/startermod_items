local resources = {}
local singleton = {}

function resources.__init()
   singleton._cached_json = {}
end

function resources.load_animation(uri)
   return _radiant.res.load_animation(uri)
end

function resources.get_mod_list()
   return _host:get_mod_list();
end

function resources.load_json(uri, enable_caching)
   local json
   if enable_caching then
      -- the json loader will convert the json object to a lua table, which can be really
      -- expensive for large json objects.  maybe lua objects would get created, etc.  so
      -- cache it so we only do that conversion once.
      json = singleton._cached_json[uri]
   end
   if not json then
      json = _radiant.res.load_json(uri)
      if enable_caching then
         singleton._cached_json[uri] = json
      end
   end
   return json
end

function resources.load_manifest(uri)
   return _radiant.res.load_manifest(uri)
end

function resources.convert_to_canonical_path(uri)
   return _radiant.res.convert_to_canonical_path(uri)
end

resources.__init()
return resources
