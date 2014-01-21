App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();

      /*
      radiant.call('stonehearth:get_world_generation_progress')
            .done(function(o) {
                  this.trace = radiant.trace(o.tracker)
                     .progress(function(result) {
                        self.updateProgress(result);
                     })
            });  

      radiant.call('stonehearth:new_game', seed);
      */
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      $("#embarkButton").click(function() {
         self._embark();
      })
   },

   _embark: function() {
      var x = 0;
      var y = 0;
      // Note: new_game accepts seeds up to 4294967295 (2^32-1)
      var seed = Math.floor(Math.random() * 10000);
      radiant.call('stonehearth:new_game', seed);
      //radiant.call('stonehearth:new_game', seed, x, y);

      App.shellView.addView(App.StonehearthLoadingScreenView);
   }

});
