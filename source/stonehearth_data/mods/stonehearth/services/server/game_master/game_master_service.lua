local game_master_lib = require 'lib.game_master.game_master_lib'
local Node = require 'services.server.game_master.controllers.node'

local GameMasterService = class()
radiant.mixin(GameMasterService, Node)

function GameMasterService:initialize()
   self._log = radiant.log.create_logger('game_master')
   self._is_enabled = radiant.util.get_config('game_master.enabled', true)

   self._sv = self.__saved_variables:get_data() 
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.disabled = {}
      self._sv.campaigns = {}
      self._sv.running_campaigns = {}
      self._sv.__self = self;

      game_master_lib.create_context('root', self)
      self._sv.ctx.player_id = 'player_1'

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
   self:_start_campaign('trader')
   self:_start_campaign('ambient_threats')
end

-- This function initializes a debug hook fn that can be used to jump ahead in 
-- a campaign for testing purposes only. 
-- @param name - the name of the target campaign, like 'combat'
-- @param debug_hook_fn - a fn that takes, as it's first argument, the name of an in-edge
--                         and that returns the appropriate out-edge
-- For examples see goblin_campaign_autotests.lua
function GameMasterService:debug_campaign(name, debug_hook_fn)
   self._debug_hook_fn = debug_hook_fn
   self:_start_campaign(name)   
end

-- Used by the various components of the GMService (campaign, arc, etc) to 
-- figure out what node to call next. If there IS a debug fn that specifies
-- something, it will produce the correct result
function GameMasterService:call_debug_hook(...)
   if not self._debug_hook_fn then
      return
   end
   return self._debug_hook_fn(...)
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
   local ctx = game_master_lib.create_context(name, campaign, self)
   ctx.campaign = campaign
   campaign:start()
end

function GameMasterService:get_root_node_command(session, response)
   return self._sv
end

return GameMasterService
