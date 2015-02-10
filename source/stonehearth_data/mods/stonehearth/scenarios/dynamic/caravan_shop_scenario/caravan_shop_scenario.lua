local CaravanShopScenario = class()
local rng = _radiant.csg.get_default_rng()

function CaravanShopScenario:initialize()
   -- xxx hack in the session.
   self.session = {
      player_id = 'player_1',
   }

   self:restore()
end

function CaravanShopScenario:can_spawn()
   return true
end

function CaravanShopScenario:restore()
  self._scenario_data = radiant.resources.load_json('stonehearth:scenarios:caravan_shop').scenario_data
end

function CaravanShopScenario:start()
   self._sv.shop = stonehearth.shop:create_shop(self.session, {'weapons', 'armor'})
   self:_post_bulletin()
end

function CaravanShopScenario:_post_bulletin(e)
   local title = self._scenario_data.title
   
   local bulletin_data = {
         title = self._scenario_data.title,
         shop = self._sv.shop
     }
   
   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self.session.player_id)
     :set_ui_view('StonehearthShopBulletinDialog')
     :set_callback_instance(self)
     :set_type('shop')
     :set_sticky(true)
     --:set_close_on_handle(self._sv.quest_completed)
     :set_data(bulletin_data)

   self.__saved_variables:mark_changed()
end

return CaravanShopScenario

