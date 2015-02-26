local game_master_lib = require 'lib.game_master.game_master_lib'

local ShopEncounter = class()

function ShopEncounter:start(ctx, info)
   self._log = radiant.log.create_logger('game_master.encounters.dialog_tree')   

   assert(info.name)
   assert(info.title)
   assert(info.inventory)

   local shop = stonehearth.shop:create_shop(ctx.player_id, info.name, info.inventory)
  
   local bulletin = stonehearth.bulletin_board:post_bulletin(ctx.player_id)
                          :set_ui_view('StonehearthShopBulletinDialog')
                          :set_callback_instance(self)
                          :set_sticky(true)
                          :set_data({
                              shop = shop,
                              title = info.title,
                              closed_callback = '_on_closed',
                           })
   self._sv.ctx = ctx
   self._sv.shop = shop
   self._sv.bulletin = bulletin

   self.__saved_variables:mark_changed()
end

function ShopEncounter:stop()
   self:_destroy_bulletin()
end

function ShopEncounter:_on_closed()
   self._log:debug('shop bulletin closed.  terminating encounter.')
   local ctx = self._sv.ctx
   ctx.arc:terminate(ctx)
end

function ShopEncounter:_destroy_bulletin()
   local bulletin = self._sv.bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.bulletin = nil
      self.__saved_variables:mark_changed()
   end
end

return ShopEncounter

