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

   didInsertElement: function() {
      this._progressbar = $("#progressbar")
      this._progressbar.progressbar({
         value: 0
      });

   },

   updateProgress: function(result) {
      var self = this;

      if (result.progress) {

         if (!this._timer) {
            this._timer = true;
            setTimeout( function () { self._updateMessage(); }, 2000);
         }

         this._progressbar.progressbar( "option", "value", result.progress );

         if (result.progress == 100) {
            App.gotoGame();
            App.gameView.addView(App.StonehearthCreateCampView)
            this.destroy();
         }
      }
   },

   _updateMessage : function() {
      var self = this;
      var max = 5;
      var min = 1;
      var random =  Math.floor(Math.random() * (max - min + 1)) + min;
      $('#message').html($.t("loading_map_" + random));
      setTimeout( function () { self._updateMessage(); }, 2000);
   }
});
