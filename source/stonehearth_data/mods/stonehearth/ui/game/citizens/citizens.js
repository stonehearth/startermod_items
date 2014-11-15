App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',
   classNames: ['exclusive'],
   closeOnEsc: true,

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Citizens');

      //this.components = App.population.getComponents();
      //this.set('uri', App.population.getUri();
      
      this._trace =  new StonehearthPopulationTrace();

      this._trace.progress(function(pop) {
            self.set('context.model', pop);
            self._buildCitizensArray();
         })
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
      this._super();
   },

   didInsertElement: function() {
      var self = this;  
      this._super();

      // remember the citizen for the row that the mouse is over
      this.$().on('mouseenter', '.row', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover' }); // Mouse over SFX
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
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:focus' });
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
            App.stonehearthClient.showCharacterSheet(citizen.__self);
         }
         event.stopPropagation();
      });

      // show toolbar control
      this.$().on('click', '.row', function() {  
         var selected = $(this).hasClass('selected'); // so we can toggle!
         self.$('.row').removeClass('selected');
         
         if (!selected) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click' });  // selecting which citizen you want SFX 
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
    },

    _citizenFilterFn: function (citizen) {
      return true;
    },
});
