local log = radiant.log.create_logger('bulletin')

Bulletin = class()

function Bulletin:initialize(id)
   assert(self._sv)

   self._sv.id = id
   self._sv.timestamp = radiant.gamestate.now()
   self._sv.type = 'info'
   self._sv.callbacks = {}

   self.__saved_variables:mark_changed()
end

function Bulletin:get_id()
   return self._sv.id
end

function Bulletin:get_title()
   return self._sv.title
end

function Bulletin:set_title(title)
   self._sv.title = title
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_timestamp()
   return self._sv.timestamp
end

function Bulletin:set_timestamp(timestamp)
   self._sv.timestamp = timestamp
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_type()
   return self._sv.type
end

function Bulletin:set_type(type)
   self._sv.type = type
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_entity()
   return self._sv.entity
end

function Bulletin:set_entity(entity)
   self._sv.entity = entity
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_description()
   return self._sv.description
end

function Bulletin:set_description(description)
   self._sv.description = description
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_ui_view()
   return self._sv.ui_view
end

-- name of the ui view to invoke
function Bulletin:set_ui_view(ui_view)
   self._sv.ui_view = ui_view
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:get_ui_data()
   return self._sv.ui_data
end

-- arbitrary data that will be passed to the view
function Bulletin:set_ui_data(ui_data)
   self._sv.data = ui_data
   self.__saved_variables:mark_changed()
   return self
end

-- when event_name is triggered by the UI, sobj:method_name(...) will get called
function Bulletin:set_callback(event_name, sobj, method_name)
   self._sv.callbacks[event_name] = {
      sobj = sobj, -- TODO: make this an id that we can rehydrate on load
      method_name = method_name,
   }
   self.__saved_variables:mark_changed()
   return self
end

function Bulletin:trigger_event(event_name, ...)
   local context = self._sv.callbacks[event_name]
   local sobj = context.sobj
   local method_name = context.method_name

   -- TODO: rehydrate sobj on load
   local fn = sobj[method_name]
   return fn(sobj, ...)
end

return Bulletin
