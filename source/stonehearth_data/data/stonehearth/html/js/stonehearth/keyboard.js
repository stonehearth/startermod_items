console.log('loading keyboard handler...');

$(document).ready(function(){

   $(top).bind('keyup', function(e){
      radiant.keyboard.handleKeyEvent(e);
   });

   radiant.keyboard.setFocus(null);

});



(function () {
   var RadiantKeyboardHandler = SimpleClass.extend({

      _hotkeyScope: null,

      init: function() { 

      },

      setFocus: function(element) {
         element = element || $('body');
         this._hotkeyScope = $(element);
      },

      handleKeyEvent: function(e) {
         var key = String.fromCharCode(e.keyCode).toLowerCase();

         var elements = this._hotkeyScope.find("[hotkey='" + key + "']");

         elements.each(function(i, el) {
            var element = $(el);
            if(element.is(':visible')) {
               // first visible element with this hotkey wins!
               element.click();
               return false;
            }
         });
      }

   });
   radiant.keyboard = new RadiantKeyboardHandler();
})();
