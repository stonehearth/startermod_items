// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
	templateName: 'town',
   classNames: ['flex', 'fullScreen'],

   scores: {
      'happiness' : 10, 
      'nutrition' : 10, 
      'shelter' : 10
   }, 

   init: function() {
      var self = this;
      this._super();

      radiant.call('stonehearth:get_score')
         .done(function(response){
            var uri = response.score;
            self.set('uri', uri);
         });
   },

   didInsertElement: function() {
      var self = this;
      this._super();
      this._updateScores();
   },

   _updateScores: function() {
      this.$('#netWorthBar').progressbar({
         value: 86
      });

      this._updateMeter(this.$('#overallScore'), this.scores.happiness, this.scores.happiness / 10);
      this._updateMeter(this.$('#foodScore'), this.scores.nutrition, this.scores.nutrition / 10);
      this._updateMeter(this.$('#shelterScore'), this.scores.shelter, this.scores.shelter / 10);
   },

   _updateMeter: function(element, value, text) {
      element.progressbar({
         value: value
      });

      element.find('.ui-progressbar-value').html(text);
   },

   _set_happiness: function() {
      this.scores.happiness = this.get('context.score_data.happiness.happiness');
      this.scores.nutrition = this.get('context.score_data.happiness.nutrition');
      this.scores.shelter = this.get('context.score_data.happiness.shelter');
      this._updateScores();
   }.observes('context.score_data.happiness')

});

