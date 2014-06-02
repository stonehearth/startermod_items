local log = radiant.log.create_logger('bulletin')

Bulletin = class()

function Bulletin:initialize(id)
   assert(self._sv)

   self._sv.id = id
   self._sv.creation_time = radiant.gamestate.now()
   self.__saved_variables:mark_changed()
end

-- unique id for the bulletin
function Bulletin:get_id()
   return self._sv.id
end

-- static data that describes the bulletin
-- this is typically the uri of the json file that contains the bulletin data
function Bulletin:set_data(data)
   self._sv.data = data
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_data()
   return self._sv.data
end

-- additional dynamic or non-text data that will be passed to the view
function Bulletin:set_context(context)
   self._sv.data = context
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_context()
   return self._sv.context
end

-- the object instance that created the bulletin that may also process callbacks
-- this is typically used in javascript view as:
--    radiant.call_obj(source, 'accepted', args);
function Bulletin:set_source(source)
   assert(source.__saved_variables, 'source must be a savable object')
   self._sv.source = source
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_source(source)
   return self._sv.source
end

-- not typically called, the creation time is automatically set on construction
function Bulletin:set_creation_time(creation_time)
   self._sv.creation_time = creation_time
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_creation_time()
   return self._sv.creation_time
end

return Bulletin
