console.log('loading keyboard handler...');

$(document).ready(function(){

   // When a DOM element is added to the page, track any keybinds in it's elements
   $(document).bind('DOMNodeInserted_HIDE', function (event) {
      var id = event.target.id;
      var element = $('#' + id);

      if (id != '' && element.hasAttr('hotkey')) {
         radiant.keyboard.addElement(element);
      }
   });

   $(top).bind('keyup', function(e){
      radiant.keyboard.handleKeyEvent(e);
   });

   radiant.keyboard.setFocus(null);

});



(function () {
   var RadiantKeyboardHandler = SimpleClass.extend({

      _elements: { },
      _hotkeyScope: null,

      init: function() { 

      },

      addElement: function(element) {
         this._elements[element.getAttr('hotkey')] = element;
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
