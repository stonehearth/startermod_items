App.StonehearthCreateCampView = App.View.extend({
	templateName: 'createCamp',

   didInsertElement: function() {
      if (!this.first) {
         /// xxx localize
         /*
         $(top).trigger('show_tip.radiant', {
            title : 'Choose your base camp location',
            description : '',
         });
         */

         this.first = true;         
      }
   },

   actions : {
      chooseCampLocation: function () {
         var self = this;
         radiant.call('stonehearth:choose_camp_location')
            .done(function(o) {
               console.log('camp location chosen!');

               self.destroy();
            });
      }
   }

});
