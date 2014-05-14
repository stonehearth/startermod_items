App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',

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

      // remember the citizen for the row that the mouse is over
      this.$().on('mouseenter', '.row', function() {
         var row = $(this);
         var pop = self.get('context');
         var id = row.attr('id');
         self._activeRowCitizen = pop.citizens[id]; 
      });
   },

   preShow: function() {
      this._buildCitizensArray();
   },

   actions: {
      doCommand: function(command) {
         App.stonehearthClient.doCommand(this._activeRowCitizen.__self, command);
      }
   },

   _foo2: function() {
      console.log('yo');
   }.observes('context.citizensArray.@each'),

   _foo: function() {
      console.log('yo');
   }.observes('context.citizensArray.@each.stonehearth:crafter'),

   _foo3: function() {
      console.log('yo');
   }.observes('context.citizensArray.@each.stonehearth:crafter.workshop'),

   _buildCitizensArray: function() {
      var vals = [];
      var citizenMap = this.get('context.citizens');
     
      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k != "__self" && citizenMap.hasOwnProperty(k)) {
               v.set('__id', k);

               vals.push(v);
            }
         });
      }

      this.set('context.citizensArray', vals);
    }.observes('context.citizens.[]'),

});
