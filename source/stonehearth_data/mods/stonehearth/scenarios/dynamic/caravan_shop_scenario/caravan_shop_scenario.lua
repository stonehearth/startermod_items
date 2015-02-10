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
   self:_create_shop()
   self:_post_bulletin()
end

-- Example shop spec
--  {
--     "name" : "Woodcutter Shop",
--     "inventory" : {
--        "item_category" : ["resources", "furniture", "decoration"],
--        "item_material" : "wood"
--     }
--  }
function CaravanShopScenario:_create_shop(spec)
  local shop_spec = spec
  
  -- pick a random shop spec from the scenario if one is not provided
  if not shop_spec then
    local keys = radiant.keys(self._scenario_data.shops)
    shop_spec = self._scenario_data.shops[keys[rng:get_int(1, #keys)]]
  end

  self._sv.shop = stonehearth.shop:create_shop(self.session, shop_spec.name, shop_spec.inventory)
  self.__saved_variables:mark_changed()
end


function CaravanShopScenario:_post_bulletin(e)
   local title = string.gsub(self._scenario_data.title, '__shop__', self._sv.shop:get_name())

   local bulletin_data = {
         title = title,
         shop = self._sv.shop
     }
   
   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self.session.player_id)
     :set_ui_view('StonehearthShopBulletinDialog')
     :set_callback_instance(self)
     :set_sticky(true)
     --:set_close_on_handle(self._sv.quest_completed)
     :set_data(bulletin_data)

   self.__saved_variables:mark_changed()
end

return CaravanShopScenario

