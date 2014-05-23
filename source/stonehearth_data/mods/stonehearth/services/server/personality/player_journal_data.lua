local PlayerJournalData = class()

function PlayerJournalData:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.all_journal_data = {}

      --Let's do some pre-filtering so the UI doesn't have to work so hard
      self._sv.journals_by_date = {}
   end
end

--- Call to add an entry to the global table for this player
function PlayerJournalData:add_entry(entry_data)
   --Put the entry in the basic table, newest first
   --table.insert(self._sv.all_journal_data, 1, entry_data)

   --Put the entry into a table that's better sorted for the UI
   self:_add_to_date(entry_data)

   --Update the UI
   self.__saved_variables:mark_changed()
end

--- Add the data in a way that is pre-sorted by date and again by score+, score-
function PlayerJournalData:_add_to_date(entry_data)
   local todays_date = entry_data.date
   if #self._sv.journals_by_date == 0 or self._sv.journals_by_date[1].date ~= todays_date then
      self:_init_day(todays_date)
   end
   table.insert(self._sv.journals_by_date[1].entries, entry_data)

   if entry_data.score_metadata then
      if entry_data.score_metadata.score_mod < 0 then
         --This entry caused the score to go down
         table.insert(self._sv.journals_by_date[1].score_down, entry_data)
      else
         --This entry caused the score to go up
         table.insert(self._sv.journals_by_date[1].score_up, entry_data)
      end
   end
   --self.__saved_variables:mark_changed()
end

--- Do we not have any journal entries for this day yet? If so create a new bucket for the day
function PlayerJournalData:_init_day(day)
   local new_day_data = {}
   new_day_data.date = day
   new_day_data.entries = {}
   new_day_data.score_up = {}
   new_day_data.score_down = {}

   table.insert(self._sv.journals_by_date, 1, new_day_data)
end

--- Returns an array of journal entries, most recent first
function PlayerJournalData:get_all_journal_data()
   return self._sv.all_journal_data
end

--- Returns an array of journal entries, sorted into buckets by date
function PlayerJournalData:get_journals_by_date()
   return self._sv.journals_by_date
end

return PlayerJournalData