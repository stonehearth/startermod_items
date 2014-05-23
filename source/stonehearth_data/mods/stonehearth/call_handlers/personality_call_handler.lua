--[[
   Stores functions related to getting data about people
]]

local PersonalityCallHandler = class()

function PersonalityCallHandler:get_journals(session, response)
   local journal_data = stonehearth.personality:get_journals_for_player(session.player_id)
   return {journals = journal_data}
end

return PersonalityCallHandler