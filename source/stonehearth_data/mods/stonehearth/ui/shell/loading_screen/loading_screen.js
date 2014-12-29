App.StonehearthLoadingScreenView = App.View.extend({
   templateName: 'stonehearthLoadingScreen',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();
      var self = this;

      radiant.call('stonehearth:get_world_generation_progress')
         .done(function(o) {
            self.trace = radiant.trace(o.tracker)
               .progress(function(result) {
                  self.updateProgress(result);
               })
         });
   },

   destroy: function() {
      if (this.trace) {
         this.trace.destroy();
         this.trace = null;
      }
   },

   didInsertElement: function() {
      this._loadTipOfTheDay();

      this._progressbar = $("#progressbar")
      this._progressbar.progressbar({
         value: 0
      });

      var numBackgrounds = 6;
      var imageUrl = '/stonehearth/ui/shell/loading_screen/images/bg' + Math.floor((Math.random()*numBackgrounds)) +'.jpg';
      this.$('#randomScreen').css('background-image', 'url(' + imageUrl + ')');

      if (this.get('hideProgress')) {
         this.$('#controls').hide();
         this.$('#text').show();
      }

   },

   _loadTipOfTheDay: function() {
      var self = this;

      $.getJSON('/stonehearth/ui/data/tips.json', function(data) {
         var random =  Math.floor(Math.random() * data.tips.length);
         self.set('tips', data.tips[random]);
      });
   },

   updateProgress: function(result) {
      var self = this;

      if (result.progress) {

         self._updateMessage();
         this._progressbar.progressbar( "option", "value", result.progress );

         if (result.progress == 100) {
            self.trace.destroy();
            self.trace = null;

            radiant.call('stonehearth:embark_client')
               .done(function(o) {
                  App.gotoGame();

                  radiant.call('radiant:get_config', 'tutorial')
                     .done(function(o) {
                        if (o.tutorial && o.tutorial.hideStartingTutorial) {
                           App.gameView.addView(App.StonehearthCreateCampView, { hideStartingTutorial: true});
                        } else {
                           App.gameView.addView(App.StonehearthHelpCameraView)
                        }
                     });
                  self.destroy();
               });
         }
      }
   },

   _updateMessage : function() {
      var self = this;
      var max = 29;
      var min = 1;
      var random =  Math.floor(Math.random() * (max - min + 1)) + min;
      this.$('#message').html($.t("loading_map_" + random));
   }
});
