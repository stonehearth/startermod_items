local log = radiant.log.create_logger('bulletin')

Bulletin = class()

function Bulletin:initialize(id)
   assert(self._sv)

   self._sv.id = id
   self._sv.creation_time = radiant.gamestate.now()
   self._sv.type = 'info'
   self._sv.close_on_handle = true
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

-- sticky bulletin notifications stay up in the UI until the user addresses them
function Bulletin:set_sticky(sticky)
   self._sv.sticky = sticky
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_ui_view()
   return self._sv.ui_view or 'StonehearthGenericBulletinDialog'
end

function Bulletin:set_ui_view(ui_view)
   self._sv.ui_view = ui_view
   self.__saved_variables:mark_changed()
   return self
end

-- data used to drive what is shown in the bulletin notification and bulletin dialog
function Bulletin:set_data(data)
   self._sv.data = data

   --Make the parent list update too?
   stonehearth.bulletin_board:update(self._sv.id)
   self.__saved_variables:mark_changed()

   return self
end

-- xxx: this function goes against the grain of the rest of the api.  either rename
-- it or make it take a function which modifies the data!
function Bulletin:modify(data)
   for k,v in pairs(data) do 
      self._sv.data[k] = v 
   end
   
   stonehearth.bulletin_board:update(self._sv.id)
   self.__saved_variables:mark_changed()

   return self
end

-- should be called from the user if anything inside the blob the sent
-- into :set_data() changes afterwards that should cause a repaint on the
-- client
function Bulletin:mark_data_changed()
   self.__saved_variables:mark_changed()
end

function Bulletin:set_close_on_handle(close_on_handle)
   self._sv.close_on_handle = close_on_handle
   self.__saved_variables:mark_changed()
   return self
end

-- keeps the dialog open while the user is interacting with it.
-- otherwise, the dialog automatically closes when the user clicks
-- the ok button (or accept, decline, etc.)
--
function Bulletin:set_keep_open(keep_open)
   self._sv.keep_open = keep_open
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_data()
   return self._sv.data
end

-- the object instance that created the bulletin that may also process callbacks
-- must be a savable object
function Bulletin:set_callback_instance(callback_instance)
   assert(self:_is_savable_object(callback_instance))

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

function Bulletin:_is_savable_object(object)
   return object and 
          object.__saved_variables and
          object.__saved_variables.get_type_name and
          object.__saved_variables:get_type_name() == 'class radiant::om::DataStore'
end

return Bulletin
