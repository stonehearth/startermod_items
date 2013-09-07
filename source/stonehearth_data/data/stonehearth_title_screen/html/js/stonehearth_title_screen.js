App.StonehearthTitleScreenView = App.View.extend({
   templateName: 'stonehearthTitleScreen',
   i18nNamespace: 'stonehearth_title_screen',
   components: {},

   newGame: function() {
      App.gotoGame();
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

   },

   credits: function() {

   }
});
