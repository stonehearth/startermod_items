// The view that shows a list of citizens and lets you promote one
App.StonehearthScoreView = App.View.extend({
	templateName: 'score',

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

      radiant.call('stonehearth:get_score')
         .done(function(response){
            var uri = response.score;
            self.set('uri', uri);
         });
   },

   _set_happiness: function() {
      this.scores.happiness = this.get('context.score_data.happiness.happiness');
      this.scores.nutrition = this.get('context.score_data.happiness.nutrition');
      this.scores.shelter = this.get('context.score_data.happiness.shelter');
      this._updateScores();
   }.observes('context.'),

   _set_worth: function() {
      this.scores.net_worth_percent = this.get('context.score_data.net_worth.percentage');
      this.scores.net_worth_total = this.get('context.score_data.net_worth.total_score');
   }.observes('context.score_data.net_worth')

});

