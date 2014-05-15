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

      //TODO: Set this from something
      this.$('#netWorthBar').progressbar({
         value: this.scores.net_worth_percent
      });

      this.$('#overallScore').progressbar({
         value: this.scores.happiness
      });
      this.$('#foodScore').progressbar({
         value: this.scores.nutrition
      });
       this.$('#shelterScore').progressbar({
         value: this.scores.shelter
      });
    }
   },

   _set_happiness: function() {
      this.scores.happiness = this.get('context.score_data.happiness.happiness');
      this.scores.nutrition = this.get('context.score_data.happiness.nutrition');
      this.scores.shelter = this.get('context.score_data.happiness.shelter');
   }.observes('context.score_data.happiness'),

   _set_worth: function() {
      this.scores.net_worth_percent = this.get('context.score_data.net_worth.percentage');
      this.scores.net_worth_total = this.get('context.score_data.net_worth.total_score');
   }.observes('context.score_data.net_worth')

});

