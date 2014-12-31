local NodeList = class()

function NodeList:initialize(name, expected_type, nodelist)
   self._sv.name = name
   self._sv.retired_nodes = {}

   self:_create_logger()
   self:_create_nodes(expected_type, nodelist)
end

function NodeList:restore()
   self:_create_logger()
end

-- elect a node in the list, filtering out those which don't pass
-- `filter_fn`
--
function NodeList:elect_node(filter_fn)
   local name = self:_run_election(filter_fn)
   if not name then
      return
   end
   
   local node = self._sv.nodes[name]   
   
   self._running_node = node
   self._sv.retired_nodes[name] = true
   self.__saved_variables:mark_changed()

   return name, node
end

-- create a logger using the dotted path to this nodelist as a prefix
--
function NodeList:_create_logger()
   self._log = radiant.log.create_logger('game_master')
                              :set_prefix(self._sv.name)
end

-- create the controllers for all the nodes in `nodelist`
--
function NodeList:_create_nodes(expected_type, nodelist)
   if not self._sv.nodes then
      local nodes = {}
      self._sv.nodes = nodes
      for name, file in pairs(nodelist) do
         nodes[name] = self:_create_node(name, expected_type, file)
      end
      self.__saved_variables:mark_changed()
   end
   return self
end

-- create and initialize a single node
--
function NodeList:_create_node(name, expected_type, file)
   local info = radiant.resources.load_json(file)
   if not info then
      self._log:error('failed to load %s "%s"', expected_type, file)
      return
   end
   local t = info.type
   if t == nil then
      self._log:error('"type" field missing in %s "%s"', expected_type, file)
      return
   end
   if t ~= expected_type then
      self._log:error('got "%s" type for "%s" (expected "%s")', t, file, expected_type)
      return
   end
   local node_controller_name = 'stonehearth:game_master:' .. expected_type
   local node = radiant.create_controller(node_controller_name, info)
   if not node then
      self._log:error('failed to load controller "%s" for "%s"', node_controller_name, file)
      return
   end

   node:set_name(self._sv.name .. '.' .. name)
   
   return node
end

-- worker to run the election.  nodes that are retiried (i.e. have been elected before)
-- and ones which do not pass `filter_fn` are disqualified
--
function NodeList:_run_election(filter_fn)
   local pop = {}
   local total = 0
   local retired_nodes = self._sv.retired_nodes
   
   for name, candidate in pairs(self._sv.nodes) do      
      if not retired_nodes[name] then
         if not filter_fn or filter_fn(name, candidate) then
            local votes = candidate:get_vote_count()
            pop[name] = votes
            total = total + votes
         end
      end
   end

   -- xxx: first node in the list always wins, for now.   
   return next(pop) -- lol!
end

return NodeList

