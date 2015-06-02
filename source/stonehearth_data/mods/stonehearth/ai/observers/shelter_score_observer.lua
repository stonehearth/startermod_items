--[[
   Based on a series of conditions, raise/lower the shelter score
   - raise it to a cap of 5 when we sleep in a bed
   - lower it whenever we sleep on the ground
   - TODO: raise score when the bed is a comfy bed
   - TODO: raise score when we fall asleep indoors
]]

local ShelterScoreObserver = class()

--Called once on creation
function ShelterScoreObserver:initialize(entity)
   self._sv.entity = entity
end

--Always called. If restore, called after restore.
function ShelterScoreObserver:activate()
   self._entity = self._sv.entity
   self._score_component = self._entity:add_component('stonehearth:score')

   self._sleep_in_bed_listener = radiant.events.listen(self._entity, 'stonehearth:sleep_in_bed', self, self._on_sleep_in_bed)
   self._sleep_on_ground_listener = radiant.events.listen(self._entity, 'stonehearth:sleep_on_ground', self, self._on_sleep_on_ground)
end

function ShelterScoreObserver:destroy()
   self._sleep_in_bed_listener:destroy()
   self._sleep_in_bed_listener = nil

   self._sleep_on_ground_listener:destroy()
   self._sleep_on_ground_listener = nil
end

function ShelterScoreObserver:_on_sleep_in_bed(e)
   local shelter_score = self._score_component:get_score('shelter')
   if shelter_score < stonehearth.constants.score.shelter.BED_SCORE_CAP then
      local journal_data = {entity = self._entity, description = 'sleep_in_bed', tags = {'shelter', 'ground'}}
      self._score_component:change_score('shelter', stonehearth.constants.score.shelter.SLEEP_IN_BED, journal_data)
   else
      --TODO: when we have comfy beds, 
      --include an else that bumps the score when it's over 5 and it's a comfy bed

      --Put in the journal entry for dreams
      local journal_data = {entity = self._entity, description = 'dreams', tags = {'sleep', 'dream'}}
      stonehearth.personality:log_journal_entry(journal_data)   
   end
end

function ShelterScoreObserver:_on_sleep_on_ground(e)
   local journal_data = {entity = self._entity, description = 'sleep_on_ground', tags = {'shelter', 'ground', 'discomfort'}}
   self._score_component:change_score('shelter', stonehearth.constants.score.shelter.SLEEP_ON_GROUND, journal_data)
end

return ShelterScoreObserver   