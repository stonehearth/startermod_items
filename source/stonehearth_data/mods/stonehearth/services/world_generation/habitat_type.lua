local HabitatType = {}

-- initial definitions
-- these will almost certainly change
HabitatType.Occupied  = 0
HabitatType.Grassland = 1
HabitatType.Foothills = 2
HabitatType.Mountains = 3
HabitatType.Forest    = 4

local string_enum_map = {
   occupied  = HabitatType.Occupied,
   grassland = HabitatType.Grassland,
   foothills = HabitatType.Foothills,
   mountains = HabitatType.Mountains,
   forest    = HabitatType.Forest
}

function HabitatType.parse_string(string)
   -- could return HabitatType[string], but we want the json to be all lower case
   return string_enum_map[string]
end

function HabitatType.parse_string_array(strings)
   local habitat_types = {}
   local habitat_type

   for _, value in pairs(strings) do
      habitat_type = HabitatType.parse_string(value)
      table.insert(habitat_types, habitat_type)
   end

   return habitat_types
end

return HabitatType
