local game_master_lib = require 'lib.game_master.game_master_lib'
local Node = require 'services.server.game_master.controllers.node'

local Campaign = class()
radiant.mixin(Campaign, Node)

local TRIGGER = 'trigger'
local CHALLENGE = 'challenge'
local CLIMAX = 'climax'

local arc_progression_states = {
   [TRIGGER] = CHALLENGE,
   [CHALLENGE] = CLIMAX,
   [CLIMAX] = nil
}

-- Assume that campaigns have 3 parts, trigger, challenge, and climax
-- challenge and climax are optional

function Campaign:initialize(info)
   self._sv._info = info
   self._sv._nodelists = {}
end

-- start the campaign
--
function Campaign:start()
   self:_select_arc_of_type(TRIGGER)
end

-- create the arc nodelist for arc with `key` (e.g. trigger, climax)
--
function Campaign:_create_arc_nodelist(key)
   local nodelist = self._sv._nodelists[key]
   if not nodelist then
      nodelist = self:_create_nodelist('arc', self._sv._info.arcs[key])
      self._sv._nodelists[key] = nodelist
      self.__saved_variables:mark_changed()
   end
   return nodelist
end

function Campaign:finish_arc(ctx)
   local arc = ctx.arc

   --what arc was this, trigger, challenge, climax? 
   local arc_type = ctx.arc_type
   local next_type = arc_progression_states[arc_type]
   if next_type then
      self:_select_arc_of_type(next_type, ctx)
   else
      --TODO: inform GM that the campaign is done? 
   end
end

-- Given a type of arc (trigger, challenge, climax, etc) pick an actual
-- arc of that subtype and start it. Set up listeners, etc
-- If applicable, stash the context of the previous arc in the ctx of this one.
-- TODO: may need to pass a filter function to the arc, based on the data?
-- For example, select aggressive vs appeasement campaign?
-- TODO: pass data into the arc so the next arc has some data about the previous arc
function Campaign:_select_arc_of_type(target_arc_type, data)
   local arcs_of_type = self:_create_arc_nodelist(target_arc_type)
   local name, arc = arcs_of_type:elect_node()
   if not arc then 
      --TODO: inform GM that the campaign is done?
      return
   end
   self._sv[target_arc_type] = arc
   self.__saved_variables:mark_changed()

   local arc_ctx = game_master_lib.create_context(target_arc_type, arc, self)
   arc_ctx.arc = arc
   arc_ctx.arc_type = target_arc_type

   arc:start()
   return arc
end

return Campaign

