App.StonehearthSquadsView = App.View.extend({
	templateName: 'squads',
   //classNames: ['exclusive'],
   closeOnEsc: true,

   init: function() {
      var self = this;
      this._super();
      this.set('title', 'Squads');
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

});
