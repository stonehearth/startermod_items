
CharacterControllerService = class()

function CharacterControllerService:__init()
   self._action_table = radiant.resources.load_json('/thunderdome/data/actions.json')
   self._frame_data = self._action_table.frame_data
end

function CharacterControllerService:set_players(p1_id, p2_id)
   self._p1 = radiant.entities.get_entity(p1_id)
   self._p2 = radiant.entities.get_entity(p2_id)
end

function CharacterControllerService:do_action(player_num, action, response)
   local entity
   local partner

   if player_num == 1 then
      entity = self._p1
      partner = self._p2
   else
      entity = self._p2
      partner = self._p1
   end
   
   
   radiant.events.trigger(entity, 'thunderdome:run_effect', { 
         effect = action
      })

   radiant.events.trigger(partner, 'thunderdome:run_effect', { 
         effect = response, 
         start_time = self:get_delay(action, response)
      })

   --local inventory = radiant.mods.load('stonehearth').inventory:get_inventory(session.faction)
   --local stockpile = inventory:create_stockpile(location, size)
   --return { stockpile = stockpile }
end

function CharacterControllerService:get_delay(effect, response_effect)
   local ms_per_frame = 1000/30
   local t1 = self._frame_data[effect]['active_frame'] * ms_per_frame
   local t2 = self._frame_data[response_effect]['active_frame'] * ms_per_frame

   local diff = math.floor(t1 - t2)
   return diff
end


return CharacterControllerService()