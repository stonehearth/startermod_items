App.StonehearthNewGameView = App.View.extend({
   templateName: 'stonehearthNewGame',
   i18nNamespace: 'stonehearth',

   modal: true,

   actions : {
      startNewGame: function() {
         var self = this;
         radiant.call('stonehearth:new_game')
            .done(function(o) {
               console.log('stonehearth:newgame came back with result: ' + o);
               App.gotoGame();
               self.buildStartingCamp();
            });         
      }
   },

   buildStartingCamp: function() {
      App.gameView.addView(App.StonehearthCreateCampView)
      this.destroy();
   }
});
