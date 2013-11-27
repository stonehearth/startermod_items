App.StonehearthTitleScreenView = App.View.extend({
   templateName: 'stonehearthTitleScreen',
   i18nNamespace: 'stonehearth',
   components: {},

   init: function() {
      this._super();
      //Start playing some kind of bgm. 
      radiant.call('radiant:play_bgm', {'track': 'stonehearth:music:title_screen'} );      
   },

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
