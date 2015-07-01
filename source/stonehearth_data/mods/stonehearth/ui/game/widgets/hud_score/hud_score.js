App.StonehearthHudScoreWidget = App.View.extend({
   templateName: 'hudScore',

   init: function() {
      var self = this;
      this._super();

      /*radiant.call('stonehearth:get_town')
         .done(function(response) {
            self.townTrace = new StonehearthDataTrace(response.town, {});
            self.townTrace.progress(function(eobj) {
                  console.log(eobj);
                  self.set('context.town', eobj);
               });
         });*/

      radiant.call('stonehearth:get_score')
         .done(function(response){
            self.scoreTrace = new StonehearthDataTrace(response.score, {});
            self.scoreTrace.progress(function(eobj) {
                  self.set('context.aggregate', eobj.aggregate);
                  if (eobj.aggregate.happiness) {
                     self._convertHappiness(eobj.aggregate.happiness);
                  }
               });
         });


      App.population.getTrace()
         .progress(function(pop) {
            self.set('context.num_citizens', Object.keys(pop.citizens).length);
         });
   },

   destroy: function() {
      if (this.townTrace) {
         this.townTrace.destroy();

      }
      if (this.scoreTrace) {
         this.scoreTrace.destroy();
      }
      this._super();
   },

   didInsertElement: function() {
      this.$('[title]').tooltipster();

      this.$().click(function() {
         App.stonehearthClient.showTownMenu();
      });
   },

   _convertHappiness: function(happiness) {
      var iconValue = Math.floor(happiness / 10); // value between 1 and 10
      var meterValue = Math.floor(happiness % 10); // value beween 1 and 10

      this.set('context.happinessIconClass', 'happiness_' + iconValue);
      this.set('context.happinessMeterClass', 'happiness_' + meterValue);
   },

});

