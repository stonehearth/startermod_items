local browser_data = stonehearth.browser_data

local BrowserDataStoreHandler = class()

function BrowserDataStoreHandler:save_browser_object(session, response, key, value)
  browser_data:save_browser_data(key, value)
end

function BrowserDataStoreHandler:load_browser_object(session, response, key)
  local value = browser_data:load_browser_data(key)
  return {value = value}
end

return BrowserDataStoreHandler
