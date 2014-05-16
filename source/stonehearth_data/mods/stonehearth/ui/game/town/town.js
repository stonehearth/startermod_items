// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
	templateName: 'town',
   classNames: ['flex', 'fullScreen'],
   //modal: true,

   scores: {
      'net_worth_percent' : 10,
      'net_worth_total': 0,
      'happiness' : 10, 
      'nutrition' : 10, 
      'shelter' : 10
   }, 

   init: function() {
      var self = this;
      this._super();
      this.showing = true;
      this._set_uri_from_call();
   },

   hide_self: function() {
      this.showing = false;
      //if we're hidden, stop getting updates
      this.set('uri', '');
   },

   show_self: function() {
      this.showing = true;
      this._set_uri_from_call();
   },

   _set_uri_from_call: function () {
      var self = this;
      radiant.call('stonehearth:get_score')
         .done(function(response){
            var uri = response.score;
            self.set('uri', uri);
         });
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      if (!this.showing) {
         this.$().hide();
      } else {
         this._updateScores();
      }
   },

   _updateScores: function() {
      this.$('#netWorthBar').progressbar({
          value: this.scores.net_worth_percent
      });

      this._updateMeter(this.$('#overallScore'), this.scores.happiness, this.scores.happiness / 10);
      this._updateMeter(this.$('#foodScore'), this.scores.nutrition, this.scores.nutrition / 10);
      this._updateMeter(this.$('#shelterScore'), this.scores.shelter, this.scores.shelter / 10);
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
   }.observes('context.score_data.net_worth')

});

