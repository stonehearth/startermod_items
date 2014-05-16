App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Citizens');
      App.population.getTrace()
         .progress(function(pop) {
            self.set('context.model', pop)
         })
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      // remember the citizen for the row that the mouse is over
      this.$().on('mouseenter', '.row', function() {
         var row = $(this);
         var pop = self.get('context.model');
         var id = row.attr('id');
         self._activeRowCitizen = pop.citizens[id]; 
      });
   },

   actions: {
      doCommand: function(command) {
         App.stonehearthClient.doCommand(this._activeRowCitizen.__self, command);
      }
   },

   getSelectedCitizen: function() {
      var citizenMap = this.get('context.model.citizens');
      var id = this.$('.selected').attr('id');
      return citizenMap[id];
   },

   _buildCitizensArray: function() {
      var self = this;
      var vals = [];
      var citizenMap = this.get('context.model.citizens');

      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k != "__self" && citizenMap.hasOwnProperty(k) && self._citizenFilterFn(v)) {
               v.set('__id', k);

               vals.push(v);
            }
         });
      }

      this.set('context.model.citizensArray', vals);
    }.observes('context.model.citizens.[]').on('init'),

    _citizenFilterFn: function (citizen) {
      return true;
    },

});
