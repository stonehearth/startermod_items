App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   // Game options (such as peaceful mode, etc.)
   options: {},

   init: function() {
      this._super();
      var self = this;
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      $('body').on( 'click', '#embarkButton', function() {
         self._embark();
      });

      $(document).on('keydown', this._clearSelectionKeyHandler);
   },

   destroy: function() {
      $(document).off('keydown', this._clearSelectionKeyHandler);
      this._super();
   },

   _embark: function() {
      var self = this;
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'} );
      App.shellView.addView(App.StonehearthSelectSettlementView);
      self.destroy();
   }
});
