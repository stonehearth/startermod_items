local Node = require 'services.server.game_master.controllers.node'

local Arc = class()
mixin_class(Arc, Node)

function Arc:initialize(info)
   self._sv.info = info
   self._sv.running_encounters = {}
end

function Arc:restore()
end

-- start the arc.  this simply starts the encounter with the `start` edge_in
--
function Arc:start(ctx)
   self._sv.ctx = ctx
   self._sv.ctx.arc = self
   self._sv.encounters = self:_create_nodelist('encounter', self._sv.info.encounters)
   self:_trigger_edge('start')
end

-- callback for encounters.  triggers the start of another encounter pointed to
-- by the out_edge of the calling encounter.
--
function Arc:trigger_next_encounter(ctx)
   local next_edge = ctx.encounter:get_out_edge()
   self:_trigger_edge(next_edge)
end

-- start the encounter which has an in_edge matching `edge_name`
--
function Arc:_trigger_edge(edge_name)
   local running_encounters = self._sv.running_encounters
   local name, encounter = self._sv.encounters:elect_node(function(name, node)
         if node:get_in_edge() ~= edge_name then
            return false
         end
         if running_encounters[name] ~= nil then
            return false
         end
         return true
      end)

   if not encounter then
      return
   end
   self:_start_encounter(name, encounter)
end

-- start an encounter.  creates an encounter specific ctx so we can recognize
-- it in the arc callbacks.
--
function Arc:_start_encounter(name, encounter)
   local ctx = self:_copy_ctx()
   ctx.encounter = encounter
   ctx.encounter_name = name

   encounter:start(ctx)

   self._sv.running_encounters[name] = encounter
   self.__saved_variables:mark_changed()
end

return Arc
