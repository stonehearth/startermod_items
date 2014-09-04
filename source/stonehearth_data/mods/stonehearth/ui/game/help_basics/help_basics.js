var StonehearthHelpBasics;

(function () {
   StonehearthHelpBasics = SimpleClass.extend({

      init: function() {
         var self = this;
               
         radiant.call('radiant:get_config', 'hide_help.basics')
            .done(function(o) {
               //self._start();
            });
      },

      _start: function() {
         var self = this;

         App.stonehearthClient.showTip('Click the Harvest button to gather resources');
         self._highlightElement('#harvest_menu');
      },

      _highlightElement: function(selector) {
         var el = $(selector)[0];

         if (this._highlight) {
            this._highlight.destroy();
            this._highlight = null;
         }

         this._highlight = App.gameView.addView(App.StonehearthElementHighlight, { element : el });
      }


   });
})();
