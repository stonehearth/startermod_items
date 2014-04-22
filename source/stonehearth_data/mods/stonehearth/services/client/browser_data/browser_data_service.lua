local log = radiant.log.create_logger('browser_data_service')

local BrowserDataService = class()

function BrowserDataService:initialize()
  self._sv = self.__saved_variables:get_data()
end

function BrowserDataService:save_browser_data(key, value)
  self._sv[key] = value
  self.__saved_variables:mark_changed()
end

function BrowserDataService:load_browser_data(key)
  return self._sv[key]
end

return BrowserDataService
