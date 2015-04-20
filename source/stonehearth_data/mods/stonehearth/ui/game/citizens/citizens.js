App.StonehearthCitizensView = App.View.extend({
	templateName: 'citizens',
   //classNames: ['exclusive'],
   closeOnEsc: true,

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Citizens');

      //this.components = App.population.getComponents();
      //this.set('uri', App.population.getUri();
      
      this._trace =  new StonehearthPopulationTrace();

      this._trace.progress(function(pop) {
            self.set('model', pop);
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
      });

      // move camera control
      this.$().on('click', '.moveCameraButton', function(event) {
         // select the row
         var row = $(this).closest('.row');

         // focus the camera to the selected citizen
         var citizen = self._rowToCitizen(row);
         
         if (citizen) {
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

      this.$().on('click', '.menuButton', function() {  
         var citizen = self._rowToCitizen($(this));

         // unselect the selected row, unconditionally. This prevents confusing things like the menu coming up
         // for one citizen, and the top toolbar appearing for the 2nd.
         self.$('.row').removeClass('selected');         
         self.set('selectedCitizen', null);
         self.set('activeCitizen', citizen);

         var menuUp = $(this).hasClass('active');

         self.$('.menuButton').removeClass('active');
         
         if (!menuUp) {
            $(this).addClass('active');      
         }
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click' });  // selecting which citizen you want SFX          
      });

      // show toolbar control
      this.$().on('click', '.row', function() {  
         var el = $(this);
         var citizen = self._rowToCitizen(el);

         // clear all menus, unconditionally
         self.$('.menuButton').removeClass('active');
         
         self.set('selectedCitizen', citizen);

         var selected = el.hasClass('selected');
         self.$('.row').removeClass('selected');
         el.addClass('selected');   
         
         if (citizen) {
            radiant.call('stonehearth:camera_look_at_entity', citizen.__self);
            radiant.call('stonehearth:select_entity', citizen.__self);
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:focus' });
         } else {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click' });  // selecting which citizen you want SFX 
         }
      });

   },

   actions: {
      doCommand: function(command) {  
         var citizen = this.get('selectedCitizen') || this.get('activeCitizen');
         App.stonehearthClient.doCommand(citizen.__self, command);
      }
   },

   _rowToCitizen: function(row) {
      var self = this;
      var id = row.attr('id');
      var pop = self.get('model');
      var citizen = pop.citizens[id];
      return citizen;
   },

   _buildCitizensArray: function() {
      var self = this;
      var citizenMap = this.get('model.citizens');

      var vals = radiant.map_to_array(citizenMap, function(k ,v) {
         if (!self._citizenFilterFn(v)) {
            return false;
         }
         v.set('__id', k);
      });
      this.set('model.citizensArray', vals);
    },

    _citizenFilterFn: function (citizen) {
      return true;
    },
});
