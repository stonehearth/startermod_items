// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
   templateName: 'town',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   scores: {
      'net_worth_percent' : 10,
      'net_worth_total': 0,
      'net_worth_level' : 'camp',
      'happiness' : 50, 
      'nutrition' :50, 
      'shelter' : 50
   }, 

   init: function() {
      var self = this;
      this._super();
      self.set('context.town_name', App.stonehearthClient.settlementName())

      radiant.call('stonehearth:get_score')
         .done(function(response){
            var uri = response.score;
            
            self.radiantTrace  = new RadiantTrace()
            self.trace = self.radiantTrace.traceUri(uri, {});
            self.trace.progress(function(eobj) {
                  self.set('context.score_data', eobj.score_data);
               });
         });
   },

   destroy: function() {
      this.radiantTrace.destroy();
      this._super();
   },

   didInsertElement: function() {
      var self = this;
      this._super();
      this._updateScores();

      // close button handler
      this.$('.title .closeButton').click(function() {
         self.destroy();
      });
   },

   _update_town_label: function() {
      var happiness_score_floor = Math.floor(this.scores.happiness/10);
      var settlement_size = i18n.t('stonehearth:' + this.scores.net_worth_level);
      if (settlement_size != 'stonehearth:undefined') {
         $('#descriptor').html(i18n.t('stonehearth:town_description', {
               "descriptor": i18n.t('stonehearth:' + happiness_score_floor + '_score'), 
               "noun": settlement_size
            }));
      }
   },

   _updateScores: function() {
      this.$('#netWorthBar').progressbar({
          value: this.scores.net_worth_percent
      });

      this._updateMeter(this.$('#overallScore'), this.scores.happiness, this.scores.happiness / 10);
      this._updateMeter(this.$('#foodScore'), this.scores.nutrition, this.scores.nutrition / 10);
      this._updateMeter(this.$('#shelterScore'), this.scores.shelter, this.scores.shelter / 10);

      this._update_town_label();
   },

   _updateMeter: function(element, value, text) {
      element.progressbar({
         value: value
      });

      element.find('.ui-progressbar-value').html(text.toFixed(1));
   },

   _set_happiness: function() {
      this.scores.happiness = this.get('context.score_data.happiness.happiness');
      this.scores.nutrition = this.get('context.score_data.happiness.nutrition');
      this.scores.shelter = this.get('context.score_data.happiness.shelter');
      this._updateScores();
   }.observes('context.score_data.happiness'),

   _set_worth: function() {
      this.scores.net_worth_percent = this.get('context.score_data.net_worth.percentage');
      this.scores.net_worth_total = this.get('context.score_data.net_worth.total_score');
      this.scores.net_worth_level = this.get('context.score_data.net_worth.level');
      this._updateScores();
   }.observes('context.score_data.net_worth')

});

