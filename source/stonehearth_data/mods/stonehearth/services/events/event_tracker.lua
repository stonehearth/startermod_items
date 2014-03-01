local EventTracker = class()

function EventTracker:__init()
   self._data_store = _radiant.sim.create_datastore(self)
   self._data = self._data_store:get_data()

   radiant.events.listen(stonehearth.events, 'stonehearth:new_event', self, self.on_new_event)
end

function EventTracker:get_data_store()
   return self._data_store
end

function EventTracker:on_new_event( e )
   self._data.text = e.text
   self._data.type = e.type
   self._data_store:mark_changed()
end

return EventTracker
