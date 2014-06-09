App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',
   closeOnEsc: true,

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
         self._activeRowCitizen = self._rowToCitizen($(this));
      });

      // move camera control
      this.$().on('click', '.moveCameraButton', function(event) {
         // select the row
         var row = $(this).closest('.row');

         // focus the camera to the selected citizen
         var citizen = self._rowToCitizen(row);
         if (citizen) {
            radiant.call('stonehearth:camera_look_at_entity', citizen.__self);
            radiant.call('stonehearth:select_entity', citizen.__self);
         }
         event.stopPropagation();
      });

      // show character sheet control
      this.$().on('click', '.banner', function(event) {
         // select the row
         var row = $(this).closest('.row');

         // focus the camera to the selected citizen
         var citizen = self._rowToCitizen(row);
         if (citizen) {

            $(top).trigger('show_character_sheet.stonehearth', {
               entity: citizen.__self
            });
         }
         event.stopPropagation();
      });

      // show toolbar control
      this.$().on('click', '.row', function() {        
         var selected = $(this).hasClass('selected'); // so we can toggle!
         self.$('.row').removeClass('selected');
         
         if (!selected) {
            $(this).addClass('selected');   
         }

      });

   },

   actions: {
      doCommand: function(command) {
         App.stonehearthClient.doCommand(this._activeRowCitizen.__self, command);
      }
   },

   _rowToCitizen: function(row) {
      var self = this;
      var id = row.attr('id');
      var pop = self.get('context.model');
      var citizen = pop.citizens[id];
      return citizen;
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
