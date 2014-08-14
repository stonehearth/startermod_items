App.StonehearthHudScoreWidget = App.View.extend({
   templateName: 'hudScore',

   init: function() {
      var self = this;
      this._super();

      radiant.call('stonehearth:get_score')
         .done(function(response){
            var uri = response.score;
            
            self.radiantTrace  = new RadiantTrace()
            self.trace = self.radiantTrace.traceUri(uri, {});
            self.trace.progress(function(eobj) {
                  self.set('context.score_data', eobj.score_data);
                  self._convertHappiness(eobj.score_data.happiness);
               });
         });
   },

   didInsertElement: function() {
      this.$('[title]').tooltipster();

      this.$().click(function() {
         App.stonehearthClient.showTownMenu();
      });
   },

   _convertHappiness: function(happiness) {
      var overallHappiness = happiness.happiness;
      var iconValue = Math.floor(overallHappiness / 10); // value between 1 and 10
      var meterValue = Math.floor(overallHappiness % 10); // value beween 1 and 10

      this.set('context.happinessIconClass', 'happiness_' + iconValue);
      this.set('context.happinessMeterClass', 'happiness_' + meterValue);
   },

   destroy: function() {
      if (this.radiantTrace != undefined) {
         this.radiantTrace.destroy();
      }
      this._super();
   },

});

