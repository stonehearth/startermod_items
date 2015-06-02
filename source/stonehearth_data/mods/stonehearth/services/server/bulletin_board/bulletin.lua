local log = radiant.log.create_logger('bulletin')

Bulletin = class()

function Bulletin:initialize(id)
   assert(self._sv)

   self._sv.id = id
   self._sv.creation_time = radiant.gamestate.now()
   self._sv.type = 'info'
   self._sv.shown = false
   self._sv.close_on_handle = true
   self.__saved_variables:mark_changed()
end

function Bulletin:restore()
   --If there is a zoom-to entity in data, make sure
   --the bulletin does not outlive the entity. 
   self:_listen_for_target_entity_destruction()
end

function Bulletin:activate()
   --We were a bulletin that was going to be destroyed at some future point
   if self._sv.active_duration_timer then
      self._sv.active_duration_timer:bind(function()
         stonehearth.bulletin_board:remove_bulletin(self._sv.id)
         self:_stop_duration_timer()
      end)
   end
end

function Bulletin:destroy()
   self:_stop_duration_timer()
   self:_destroy_death_listener()
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

--Set true if this bulletin has been flashed at the user yet
function Bulletin:set_shown(shown)
   self._sv.shown = shown
   self.__saved_variables:mark_changed()
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

   --If there is a zoom-to entity in data, make sure
   --the bulletin does not outlive the entity. 
   self:_listen_for_target_entity_destruction()

   --Make the parent list update too?
   stonehearth.bulletin_board:update(self._sv.id)
   self.__saved_variables:mark_changed()

   return self
end

function Bulletin:_listen_for_target_entity_destruction()
   local data = self._sv.data
   if data and data.zoom_to_entity then
      self._entity_death_listener = radiant.events.listen(data.zoom_to_entity, 'radiant:entity:pre_destroy', self, self._on_target_destroy)
   end
end

--If the bulletin is still active when the target is destroyed, remove it
function Bulletin:_on_target_destroy()
   --If the bulletin is already gone, this is a safe no-op
   stonehearth.bulletin_board:remove_bulletin(self._sv.id)
   self:_destroy_death_listener()
   self:_stop_duration_timer()
   self.__saved_variables:mark_changed()
end

function Bulletin:_destroy_death_listener()
   if self._entity_death_listener then
      self._entity_death_listener:destroy()
      self._entity_death_listener = nil
   end
end

--If you don't want the bulletin to stick around forever, set this value
function Bulletin:set_active_duration(duration)
   self._sv.active_duration_timer = stonehearth.calendar:set_timer("Bulletin remove bulletin", duration, function()
      stonehearth.bulletin_board:remove_bulletin(self._sv.id)
      self:_stop_duration_timer()
      self:_destroy_death_listener()
   end)
   self.__saved_variables:mark_changed()
end

function Bulletin:_stop_duration_timer()
   if self._sv.active_duration_timer then
      self._sv.active_duration_timer:destroy()
      self._sv.active_duration_timer = nil
      self.__saved_variables:mark_changed()
   end
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
   assert(radiant.is_controller(callback_instance))

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
