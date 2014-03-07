App.StonehearthZonesModeView = App.View.extend({
	templateName: 'zonesMode',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', 'modeBackground'],

   didInsertElement: function() {
      this.$().hide();
   }
});
