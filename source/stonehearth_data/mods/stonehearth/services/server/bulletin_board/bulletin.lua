local log = radiant.log.create_logger('bulletin')

Bulletin = class()

function Bulletin:initialize(id)
   assert(self._sv)

   self._sv.id = id
   self._sv.creation_time = radiant.gamestate.now()
   self._sv.type = 'info'
   self.__saved_variables:mark_changed()
end

-- unique id for the bulletin
function Bulletin:get_id()
   return self._sv.id
end

function Bulletin:get_type()
   return self._sv.type
end

function Bulletin:set_type(type)
   self._sv.type = type
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_ui_view()
   return self._sv.ui_view
end

function Bulletin:set_ui_view(ui_view)
   self._sv.ui_view = ui_view
   self.__saved_variables:mark_changed()
   return self
end

-- data used to drive what is shown in the bulletin notification and bulletin dialog
function Bulletin:set_data(data)
   self._sv.data = data
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_data()
   return self._sv.data
end

-- the object instance that created the bulletin that may also process callbacks
-- this is typically used in javascript view as:
--    radiant.call_obj(callback_instance, 'accepted', args);
function Bulletin:set_callback_instance(callback_instance)
   assert(callback_instance.__saved_variables, 'callback_instance must be a savable object')
   self._sv.callback_instance = callback_instance
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_callback_instance(callback_instance)
   return self._sv.callback_instance
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
