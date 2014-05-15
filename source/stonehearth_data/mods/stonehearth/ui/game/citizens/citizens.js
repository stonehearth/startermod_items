// The view that shows a list of citizens and lets you promote one
App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',

   init: function() {
      var self = this;
      this._super();

      App.population.getTrace()
         .progress(function(pop) {
            self.set('context.model', pop)
         })
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
         if ($(this).hasClass('disabled')) {
            return;
         }
         
         var selectedCitizen = self.getSelectedCitizen();
         self.promotionWizard = App.gameView.addView(App.StonehearthPromotionWizard, { 
               citizen : selectedCitizen
            });
      });
   },

   actions: {
      showWorkshop: function(crafter) {
         var workshop = crafter['stonehearth:crafter']['workshop']['workshop_entity'];
         $(top).trigger("radiant_show_workshop_from_crafter", {
            event_data: {
               workshop: workshop
            }
         });
      },
      placeWorkshop: function(crafter) {
         $(top).trigger('build_workshop', {
            event_data : {
               crafter: crafter.__self   
            }
         })
      }
   },

   getSelectedCitizen: function() {
      var citizenMap = this.get('context.model.citizens');
      var id = this.$('.selected').attr('id');
      return citizenMap[id];
   },

   _foo2: function() {
      console.log('yo');
   }.observes('context.model.citizensArray.@each'),

   _foo: function() {
      console.log('yo');
   }.observes('context.model.citizensArray.@each.stonehearth:crafter'),

   _foo3: function() {
      console.log('yo');
   }.observes('context.model.citizensArray.@each.stonehearth:crafter.workshop'),

   _buildCitizensArray: function() {
      var vals = [];
      var citizenMap = this.get('context.model.citizens');

      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k != "__self" && citizenMap.hasOwnProperty(k)) {
               v.set('__id', k);

               vals.push(v);
            }
         });
      }

      this.set('context.model.citizensArray', vals);
    }.observes('context.model.citizens.[]').on('init'),
});
