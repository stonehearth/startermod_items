App.StonehearthTitleScreenView = App.View.extend({
   templateName: 'stonehearthTitleScreen',
   i18nNamespace: 'stonehearth',
   components: {},

   actions: {
      newGame: function() {
         this.get('parentView').addView(App.StonehearthNewGameView)
         //App.gotoGame();
         /*
         $.get("...")
            .progress( function(e) {
               //update progress thingy
            }
            .done( function(e) {
               App.gotoGame();
            });
         */
      },

      loadGame: function() {
         App.gotoGame();
      },

      credits: function() {

      }
   }
});
