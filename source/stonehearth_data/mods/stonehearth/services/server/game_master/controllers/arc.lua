local game_master_lib = require 'lib.game_master.game_master_lib'
local Node = require 'services.server.game_master.controllers.node'

local Arc = class()
radiant.mixin(Arc, Node)

function Arc:initialize(info)
   self._sv._info = info
   self._sv.running_encounters = {}
end

function Arc:restore()
end

function Arc:activate()
   self._log = radiant.log.create_logger('game_master.arc')
end


-- start the arc.  this simply starts the encounter with the `start` edge_in
--
function Arc:start()
   
   self._sv.encounters = self:_create_nodelist('encounter', self._sv._info.encounters)
   self.__saved_variables:mark_changed()

   self:_trigger_edge('start', self)
end


-- callback for encounters.  triggers the start of another encounter pointed to
-- by the out_edge of the calling encounter.
--
function Arc:trigger_next_encounter(ctx)
   local encounter = ctx.encounter
   local encounter_name = ctx.encounter_name

   assert(encounter)
   assert(encounter_name)

   local out_edge = encounter:get_out_edge()
   self:_stop_encounter(encounter_name, encounter)

   if not out_edge then
      self._log:info('encounter "%s" has no out_edge.  end of the line!', encounter_name)
      return
   end

   -- if the edge is "arc:finish" then the arc has officially ended. Let
   -- parent campaign know it's OK to start another arc. 
   -- As a nicety, pass the current ctx, in case the next arc wants context
   -- Other encounters from this arc may still be around (waiting on boss death, etc)
   -- but they should resolve themselves on their own
   if out_edge == 'arc:finish' then
      ctx.campaign:finish_arc(ctx)
      return
   end

   -- trigger all the edges in the out_edge list, in order.  if there's just a single
   -- edge to trigger, we let out_edge be a string.
   if type(out_edge) == 'string' then
      out_edge = { out_edge }
   end
   for _, next_edge in pairs(out_edge) do
      self._log:info('encounter "%s" is triggering next edge "%s"', encounter_name, next_edge)
      self:_trigger_edge(next_edge, encounter)
   end
end

-- callback for encounters.  creates another encounter without finishing the current one
--
function Arc:spawn_encounter(ctx, next_edge)
   local encounter = ctx.encounter
   local encounter_name = ctx.encounter_name

   assert(encounter)
   assert(encounter_name)

   self._log:info('encounter "%s" is spawning edge "%s"', encounter_name, next_edge)
   self:_trigger_edge(next_edge, encounter)
end

-- callback for encounters.  terminate the current encounter
--
function Arc:terminate(ctx)
   local encounter = ctx.encounter
   local encounter_name = ctx.encounter_name
   assert(encounter)
   assert(encounter_name)
   self:_stop_encounter(encounter_name, encounter)
end

-- start the encounter which has an in_edge matching `edge_name`
--
function Arc:_trigger_edge(edge_name, parent_node)
   self._log:info('triggering edge "%s"', edge_name)

   local parent_ctx = parent_node:get_ctx()
   local running_encounters = self._sv.running_encounters
   local name, encounter = self._sv.encounters:elect_node(function(name, node)
         local in_edge = node:get_in_edge()
         if in_edge ~= edge_name then
            self._log:debug('skipping encounter "%s" (in edge "%s" doesn\'t match)', name, in_edge)
            return false
         end
         local unique = node:get_is_unique()
         if unique and running_encounters[name] ~= nil then
            self._log:debug('skipping encounter "%s" (already running)', name)
            return false
         end
         if not node:can_start(parent_ctx) then
            self._log:debug('skipping encounter "%s" (cannot start)', name)
            return false
         end
         self._log:debug('found candidate encounter "%s"', name)
         return true
      end)

   if not encounter then
      self._log:info('could not find encounter for edge named "%s".  bailing.', edge_name)
      return
   end
   self:_start_encounter(name, encounter, parent_node)
end

-- start an encounter.  creates an encounter specific ctx so we can recognize
-- it in the arc callbacks.
--
function Arc:_start_encounter(name, encounter, parent_node)
   self._log:info('starting encounter "%s"', name)

   local enc_ctx = game_master_lib.create_context(name, encounter, parent_node)
   enc_ctx.encounter = encounter
   enc_ctx.encounter_name = name

   encounter:start(enc_ctx)

   self._sv.running_encounters[name] = encounter
   self.__saved_variables:mark_changed()
end

function Arc:_stop_encounter(name, encounter)
   self._log:debug('stopping encounter "%s"', name)

   self._sv.running_encounters[name] = nil
   encounter:stop()
   --encounter:destroy()
end

return Arc
