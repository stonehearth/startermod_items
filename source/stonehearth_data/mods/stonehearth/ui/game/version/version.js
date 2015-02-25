
App.StonehearthVersionView = App.View.extend({
	templateName: 'versionTag',

   init: function() {
      var self = this;

      self._super();
      radiant.call('radiant:client_about_info')
         .done(function(o) {
            self.set('version', o.product_branch + '-' +  o.product_build_number + ' (' + o.architecture + ')');
         });
   }
});
