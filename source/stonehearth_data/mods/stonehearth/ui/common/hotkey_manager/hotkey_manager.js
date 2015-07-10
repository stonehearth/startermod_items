/***
 * A manager for the hotkey bindings in the game.
 ***/
var StonehearthHotkeyManager;

(function () {
   StonehearthHotkeyManager = SimpleClass.extend({

      init: function() {
         var self = this;
         self._hotkeyBindings = {}
         radiant.call('radiant:get_config', 'hotkey_bindings')
            .done(function(response) {
               if (response['hotkey_bindings']) {
                  self._hotkeyBindings = response['hotkey_bindings'];
               }
            });
      },

      getHotKey: function(hotkeyName, defaultValue) {
         if (!this._hotkeyBindings) {
            return defaultValue;
         }
         return this._hotkeyBindings[hotkeyName] ? this._hotkeyBindings[hotkeyName] : defaultValue;
      }
      
   });
})();
