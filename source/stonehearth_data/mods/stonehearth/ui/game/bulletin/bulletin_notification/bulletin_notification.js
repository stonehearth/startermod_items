App.StonehearthBulletinNotification = App.View.extend({
	templateName: 'bulletinNotification',

   didInsertElement: function() {
      // TODO: read these contsants from config
      var bulletinNotificationDuration = 5000;
      var bulletinNotificationFadeTime = 3000;
      var self = this;
      this._super();

      self.$('#bulletinNotification').pulse();

      setTimeout(function() {
         self.$('#bulletinNotification').fadeOut(bulletinNotificationFadeTime, 'swing', function() {
            self.destroy();
         });
      }, bulletinNotificationDuration);

      self.$('#popup').click(function() {
         var bulletin = self.get('context');
         App.bulletinBoard.showDialogView(bulletin);
         self.destroy();
      });
   },

   willDestroyElement: function() {
      App.bulletinBoard.onNotificationViewDestroyed();
      this._super();
   }
});
