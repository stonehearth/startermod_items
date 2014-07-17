local MemorializeDeathAction = class()

MemorializeDeathAction.name = 'memorialize death'
MemorializeDeathAction.does = 'stonehearth:memorialize_death'
MemorializeDeathAction.args = {}
MemorializeDeathAction.version = 2
MemorializeDeathAction.priority = 1

function MemorializeDeathAction:start_thinking(ai, entity, args)
   self._name = radiant.entities.get_display_name(entity)
   self._player_id = radiant.entities.get_player_id(entity)
   self._location = ai.CURRENT.location
   ai:set_think_output()
end

-- consider separating all these 'effects' into a compound action
function MemorializeDeathAction:run(ai, entity, args)
   local title = string.format('RIP %s', self._name)
   local description = string.format('%s will always be remembered', self._name)
   local event_text = string.format('%s has died.', self._name)
   local tombstone = radiant.entities.create_entity('stonehearth:tombstone')

   radiant.entities.set_name(tombstone, title)
   radiant.entities.set_description(tombstone, description)
   radiant.terrain.place_entity(tombstone, self._location)

   radiant.effects.run_effect(tombstone, '/stonehearth/data/effects/death/death_jingle.json')

   stonehearth.events:add_entry(event_text, 'warning')

   --Send the notice to the bulletin service.
   self.notification_bulletin = stonehearth.bulletin_board:post_bulletin(self._player_id)
         :set_callback_instance(stonehearth.town:get_town(self._player_id))
         :set_type('alert')
         :set_data({
            title = event_text,
            message = '',
            zoom_to_entity = tombstone,
         })

end

return MemorializeDeathAction
