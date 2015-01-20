App.StonehearthBulletinList = App.View.extend({
	templateName: 'bulletinList',
   closeOnEsc: true,

   init: function() {
      var self = this;
      self._super();
   },

   didInsertElement: function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'});
      var self = this;
      self._super();

      this.$().on('click', '.row', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:page_down'});
         var row = $(this);
         var id = row.attr('id');
         var bulletins = self.get('context');
         var bulletin;

         for (var i = 0; i < bulletins.length; i++) {
            if (bulletins[i].id == id) {
               bulletin = bulletins[i];
            }
         }

         App.bulletinBoard.zoomToLocation(bulletin);
         App.bulletinBoard.showDialogView(bulletin);
      });
   },
});
