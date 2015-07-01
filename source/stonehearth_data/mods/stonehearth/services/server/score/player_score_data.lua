--[[
   Keep a datastore for each player with score data
]]

local PlayerScoreData = class()

function PlayerScoreData:initialize(player_id)
   self._sv._initialized = true

   -- new hotness down here
   self._sv.player_id = player_id
   self._sv.contributing_entities = {}
   self._sv.total_scores = {}
   self._sv.aggregate = {} 
end

function PlayerScoreData:activate()
   radiant.events.listen(radiant, 'radiant:unit_info:player_id_changed', self, self._on_player_id_changed)
   radiant.events.listen(radiant, 'radiant:entity:pre_destroy', self, self._on_destroy)
end

function PlayerScoreData:_on_player_id_changed(e)
   local entity = e.entity
   local player_id = radiant.entities.get_player_id(entity)
   if player_id ~= self._sv.player_id then
      self:_remove_entity_from_score(entity)
   end
end

function PlayerScoreData:_on_destroy(e)
   self:_remove_entity_from_score(e.entity)
end

function PlayerScoreData:_remove_entity_from_score(entity)
   if not entity or not entity:is_valid() then
      return
   end
   local id = entity:get_id()
   local contrib = self._sv.contributing_entities[id]
   if not contrib then
      return
   end
   for category, reasons in pairs(contrib) do
      for reason, value in pairs(reasons) do
         self._sv.total_scores[category] = self._sv.total_scores[category] - value
      end
      self:_update_aggregate(category)
   end
   self._sv.contributing_entities[id] = nil
end

function PlayerScoreData:get_score_data()
   return self._sv
end

-- new hotness down here
function PlayerScoreData:change_score(entity, category_name, reason, value)
   self:_update_score(entity, category_name, reason, value)
   self:_update_aggregate(entity, category_name)
   self.__saved_variables:mark_changed()
end

function PlayerScoreData:_update_aggregate(entity, category_name)
   local pop = stonehearth.population:get_population(entity)
   if not pop then
      return
   end
   local count = pop:get_citizen_count()
   if count > 0 then
      self._sv.aggregate[category_name] = self._sv.total_scores[category_name] / pop:get_citizen_count()
   else
      self._sv.aggregate[category_name] = self._sv.total_scores[category_name]
   end
end

function PlayerScoreData:_update_score(entity, category_name, reason, value)
   local id = entity:get_id()
   local contrib = self._sv.contributing_entities[id]
   if not contrib then
      contrib = {}
      self._sv.contributing_entities[id] = contrib
   end

   local category = contrib[category_name]
   if not category then
      category = {}
      contrib[category_name] = category
   end

   local old_value = category[reason] or 0
   category[reason] = value

   local delta = value - old_value

   local current_score = self._sv.total_scores[category_name] or 0
   self._sv.total_scores[category_name] = current_score + delta
end

return PlayerScoreData

