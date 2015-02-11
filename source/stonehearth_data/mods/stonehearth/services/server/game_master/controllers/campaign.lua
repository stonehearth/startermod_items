local Node = require 'services.server.game_master.controllers.node'

local Campaign = class()
mixin_class(Campaign, Node)

function Campaign:initialize(info)
   self._sv._info = info
   self._sv._nodelists = {}
end

-- start the campaign
--
function Campaign:start(ctx)
   -- create all the triggers...
   local triggers = self:_create_arc_nodelist('trigger')
   local name, trigger = triggers:elect_node()
   if not trigger then
      return
   end
   self._sv.trigger = trigger
   self.__saved_variables:mark_changed()
   trigger:start(ctx)
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

return Campaign

