function get_time_and_date()
   return radiant.mods.require('/stonehearth_calendar').get_time_and_date()
end

return get_time_and_date
