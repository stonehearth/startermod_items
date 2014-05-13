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

      //TODO: Set this from something
      this.$('#netWorthBar').progressbar({value:50})
      
      this.$('#overallScore').progressbar({
         value: this.scores.happiness
      });
      this.$('#foodScore').progressbar({
         value: this.scores.nutrition
      });
       this.$('#shelterScore').progressbar({
         value: this.scores.shelter
      });
      
      
      
   },


   _set_happiness: function() {
      this.scores.happiness = this.get('context.score_data.happiness.happiness');
      this.scores.nutrition = this.get('context.score_data.happiness.nutrition');
      this.scores.shelter = this.get('context.score_data.happiness.shelter');
   }.observes('context.score_data.happiness')

});

