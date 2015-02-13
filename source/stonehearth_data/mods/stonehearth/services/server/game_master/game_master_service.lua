local Node = require 'services.server.game_master.controllers.node'

local GameMasterService = class()
mixin_class(GameMasterService, Node)

function GameMasterService:initialize()
   self._log = radiant.log.create_logger('game_master')
   self._is_enabled = radiant.util.get_config('game_master.enabled')

   self._sv = self.__saved_variables:get_data() 
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.disabled = {}
      self._sv.campaigns = {}
      self._sv.running_campaigns = {}
      self:set_name('gm')
   else
      self:restore()
   end  
end

function GameMasterService:restore()
end

function GameMasterService:enable_campaign_type(type, enabled)
   self._sv.disabled[type] = not enabled
   self.__saved_variables:mark_changed()
end

function GameMasterService:start()
   if not self._is_enabled then
      self._log:error('cannot start game_master.  not enabled!')
      return
   end
   self:_start_campaign('combat')
end

function GameMasterService:_create_context()
   return {
      player_id = 'player_1'
   }
end

function GameMasterService:is_enabled()
   return self._is_enabled
end

function GameMasterService:_start_campaign(subtype)
   if self._sv.running_campaigns[subtype] then
      return
   end
   assert(not self._sv.running_campaigns[subtype])

   if self._sv.disabled[subtype] then
      self._log:info('ignoring request to start "%s".  disabled.', subtype)
      return
   end

   -- xxx: there should probably be a CampaignCategory class or something
   -- to manage this.  Consider adding once we get beyond combat
   local campaigns = self._sv.campaigns[subtype]
   if not campaigns then
      local index = radiant.resources.load_json('stonehearth:data:gm_index')
      campaigns = self:_create_nodelist('campaign', index.campaigns[subtype])
      self._sv.campaigns[subtype] = campaigns
      self.__saved_variables:mark_changed()
   end

   local name, campaign = campaigns:elect_node()
   if not campaign then
      return
   end
   self._sv.running_campaigns[subtype] = campaign

   -- every campaign gets a new context which is shared among all arcs
   -- and encounters for that campaign.
   local ctx = self:_create_context()
   campaign:start(ctx)
end

function GameMasterService:get_data_command(session, response)
   return self.__saved_variables
end

return GameMasterService
