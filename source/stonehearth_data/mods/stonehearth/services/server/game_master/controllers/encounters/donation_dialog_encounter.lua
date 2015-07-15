local DonationDialogEncounter = class()
local rng = _radiant.csg.get_default_rng()
local Point3 = _radiant.csg.Point3
local LootTable = require 'stonehearth.lib.loot_table.loot_table'
local game_master_lib = require 'lib.game_master.game_master_lib'

--[[
   Use when someone wants to give you something and has some dialog to go with it. 
   (as opposed to Donation Encounter, which just drops stuff off.)
]]

function DonationDialogEncounter:initialize()
end

function DonationDialogEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.donation_dialog_encounter')
end

-- Pick the donation and prep the message
function DonationDialogEncounter:start(ctx, info)
   self._sv.player_id = ctx.player_id

   game_master_lib.compile_bulletin_nodes(info.nodes, ctx)

   --Get the drop items
   assert(info.loot_table)
   self._sv.items = LootTable(info.loot_table)
                     :roll_loot()
   local reward_items = {}
   for uri, quantity in pairs(self._sv.items) do 
      table.insert(reward_items, {count = quantity, display_name = radiant.entities.get_component_data(uri, 'unit_info').name})
   end

   --Compose the bulletin
   local bulletin_data = info.nodes.simple_message.bulletin
   bulletin_data.demands = reward_items

   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
                                       :set_ui_view('StonehearthDialogTreeBulletinDialog')
                                       :set_callback_instance(self)
                                       :set_type('quest')
                                       :set_sticky(false)
                                       :set_keep_open(false)
                                       :set_close_on_handle(true)
                                       :set_active_duration(info.expiration_timeout)
   self._sv.bulletin:set_data(bulletin_data)
   self.__saved_variables:mark_changed()
end

function DonationDialogEncounter:_on_dialog_tree_choice(session, response, choice)
   local ctx = self._sv.ctx
   local bulletin = self._sv.bulletin

   --self:_stop_timer()

   if not bulletin then
      self._log:error('no current bulletin in _on_dialog_tree_choice (choice:%s)', choice)
      return false
   end
   local data = bulletin:get_data()
   local choice = data.choices[choice]
   if not choice then
      self._log:error('unexpected choice in _on_dialog_tree_choice (choice:%s)', choice)
      ctx.arc:terminate(ctx)
      return false
   end
   if choice.result == 'accept' then
      self:_acknowledge()
      self:_destroy_bulletin()
      return true
   end
   if choice.result == 'reject' then
      self:_destroy_bulletin()
      return false
   end
end

--- Once the user has acknowledged the bulletin, add the target reward beside the banner
function DonationDialogEncounter:_acknowledge()
   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return
   end
   radiant.entities.spawn_items(self._sv.items, drop_origin, 1, 3, { owner = self._sv.player_id })
end

function DonationDialogEncounter:_destroy_bulletin()
   local bulletin = self._sv.bulletin
   if bulletin then
      stonehearth.bulletin_board:remove_bulletin(bulletin)
      self._sv.bulletin = nil
      self.__saved_variables:mark_changed()
   end
end

return DonationDialogEncounter