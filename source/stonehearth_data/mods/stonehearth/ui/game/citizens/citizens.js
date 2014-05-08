// The view that shows a list of citizens and lets you promote one
App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',

   components: {
      "citizens" : {
         "*" : {
            "stonehearth:profession" : {
               "profession_uri" : {}
            },
            "unit_info": {},
         }
      }
   },

   init: function() {
      var self = this;
      this._super();

      var pop = App.population.getData();
      this.set('context', pop);
      this._buildCitizensArray();
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      // select a row
      this.$().on('click', '.row', function() {
         self.$('.selected').removeClass('selected');
         $(this).addClass('selected');

         if ($(this).attr('profession') == 'stonehearth:professions:worker') {
            self.$('#promoteButton').removeClass('disabled');   
         } else {
         self.$('#promoteButton').addClass('disabled');   
         }
      });

      this.$('#promoteButton').click(function() {
         var selectedCitizen = self.getSelectedCitizen();
         self.promotionWizard = App.gameView.addView(App.StonehearthPromotionWizard, { 
               citizen : selectedCitizen
            });
      });
   },

   preShow: function() {

   },

   getSelectedCitizen: function() {
      var citizenMap = this.get('context.citizens');
      var id = this.$('.selected').attr('id');
      return citizenMap[id];
   },

   _buildCitizensArray: function() {
      var vals = [];
      var citizenMap = this.get('context.citizens');
     
      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k != "__self" && citizenMap.hasOwnProperty(k)) {
               v['__id'] = k;
               vals.push(v);
            }
         });
      }

      this.set('context.citizensArray', vals);
    }.observes('context.citizens.[]'),

});
