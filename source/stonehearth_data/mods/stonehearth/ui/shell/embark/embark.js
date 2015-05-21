App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   // Game options (such as peaceful mode, etc.)
   _options: {},

   init: function() {
      this._super();
      var self = this;
      this._trace = new StonehearthPopulationTrace();

      this._trace.progress(function(pop) {
            self.set('model', pop);
            self._buildCitizensArray();
         });
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      $('body').on( 'click', '#embarkButton', function() {
         self._embark();
      });

      self.$("#regenerateButton").click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:reroll'} );

         radiant.call_obj('stonehearth.game_creation', 'generate_citizens_command')
            .done(function(e) {
            })
            .fail(function(e) {
               console.error('generate_citizens failed:', e)
            });
      });

      radiant.call_obj('stonehearth.game_creation', 'generate_citizens_command')
         .done(function(e) {
         })
         .fail(function(e) {
            console.error('generate_citizens failed:', e)
         });

      $(document).on('keydown', this._clearSelectionKeyHandler);
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
      $(document).off('keydown', this._clearSelectionKeyHandler);
      this._super();
   },

   _embark: function() {
      var self = this;
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'} );
      App.shellView.addView(App.StonehearthSelectSettlementView, {options: self._options});
      self.destroy();
   },

   _buildCitizensArray: function() {
      var self = this;
      var citizenMap = this.get('model.citizens');

      var vals = radiant.map_to_array(citizenMap, function(k ,v) {
         v.set('__id', k);
      });
      this.set('model.citizensArray', vals);
   },

});
