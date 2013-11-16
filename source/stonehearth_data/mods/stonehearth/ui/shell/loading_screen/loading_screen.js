App.StonehearthLoadingScreenView = App.View.extend({
   templateName: 'stonehearthLoadingScreen',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();
      var self = this;

      radiant.call('stonehearth:get_world_generation_progress')
            .done(function(o) {
                  this.trace = radiant.trace(o.tracker)
                     .progress(function(result) {
                        self.updateProgress(result);
                     })
            });  

      radiant.call('stonehearth:new_game');
   },

   updateProgress: function(result) {
      if (result.progress) {
         $('loadingScreen').append('<div>' + progress.progress + '</div>');
      }
   },

   gotoGame: function() {
      App.gotoGame();
      App.gameView.addView(App.StonehearthCreateCampView)
      this.destroy();
   }
});
