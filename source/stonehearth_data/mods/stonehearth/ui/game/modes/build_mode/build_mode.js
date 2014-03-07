App.StonehearthBuildModeView = App.View.extend({
	templateName: 'buildMode',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', 'modeBackground'],

   didInsertElement: function() {
      this.$().hide();
   },

});
