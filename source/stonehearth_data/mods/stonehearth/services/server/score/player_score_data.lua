local PlayerScoreData = class()

function PlayerScoreData:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = false
      self._sv.score_data = {}
   end
end

function PlayerScoreData:set_happiness(happiness_data)
   self._sv.score_data.happiness = happiness_data
   self.__saved_variables:mark_changed()
end

function PlayerScoreData:get_score_data()
   return self._sv.score_data
end

return PlayerScoreData
