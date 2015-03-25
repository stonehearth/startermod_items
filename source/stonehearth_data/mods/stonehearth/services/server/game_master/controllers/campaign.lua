local game_master_lib = require 'lib.game_master.game_master_lib'
local Node = require 'services.server.game_master.controllers.node'

local Campaign = class()
mixin_class(Campaign, Node)

-- Assume that campaigns have 3 parts, trigger, challenge, and climax
-- challenge and climax are optional
-- TODO: should we make this an extensible sequence

function Campaign:initialize(info)
   self._sv._info = info
   self._sv._nodelists = {}
end

function Campaign:restore()
   if self._sv.trigger then
      self._trigger_listener = radiant.events.listen(self._sv.trigger, 'stonehearth:arc_finish', self, self._on_trigger_finish())
   end
   if self._sv.challenge then
      self._challenge_listener = radiant.events.listen(self._sv.challenge, 'stonehearth:arc_finish', self, self._on_challenge_finish)
   end
end

function Campaign:destroy()
   if self._trigger_listener then
      self._trigger_listener:destroy()
      self._trigger_listener = nil
   end
   if self._challenge_listener then
      self._challenge_listener:destroy()
      self._challenge_listener = nil
   end
end

-- start the campaign
--
function Campaign:start()
   -- create all the triggers...
   local triggers = self:_create_arc_nodelist('trigger')

   --pick one possible trigger (it's type will be arc)
   local name, trigger = triggers:elect_node()
   if not trigger then
      return
   end
   self._sv.trigger = trigger
   self.__saved_variables:mark_changed()

   game_master_lib.create_context('trigger', trigger, self)

   --tell the arc to start itself
   trigger:start()

   --listen for when the arc declares itself over and then start the "challenge" arc
   --TODO: do we want to make the order less hardcoded?
   self._trigger_listener = radiant.events.listen(trigger, 'stonehearth:arc_finish', self, self._on_trigger_finish)
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

-- When the trigger arc is finished, start a challenge arc
function Campaign:_on_trigger_finish(data)
   local arc = self:_select_arc_of_type('challenge', data)
   if arc then
      self._challenge_listener = radiant.events.listen(arc, 'stonehearth:arc_finish', self, self._on_challenge_finish)
   end
   --TODO: test, if there are outstanding nodes here, will nilling out this reference hurt them?
   self._sv.trigger = nil
   return radiant.events.UNLISTEN
end

--Given a type of arc (trigger, challenge, climax, etc) pick an actual
--arc of that subtype and start it. Set up listeners, etc
--START HERE WED: RESUE THIS FOR TRIGGER AND CLIMAX
--THEN BUILD WARG CAMPS
function Campaign:_select_arc_of_type(target_arc_type, data)
   local arcs_of_type = self:_create_arc_nodelist(target_arc_type)
   local name, arc = arcs_of_type:elect_node()
   if not arc then 
      --TODO: inform GM that the campaign is done?
      return
   end
   self._sv[target_arc_type] = arc
   self.__saved_variables:mark_changed()

   local inherited_parent = self
   
   --TODO: is it possible to pass on data from the previous campaign?
   if data.prev_node then
      inherited_parent = data.prev_node
   end
   game_master_lib.create_context(target_arc_type, arc, inherited_parent)
   arc:start()
   return arc
end

return Campaign

