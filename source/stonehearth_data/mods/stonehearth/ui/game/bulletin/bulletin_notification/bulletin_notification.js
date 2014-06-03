
$(document).ready(function(){
  
   App.stonehearth.bulletinBoard = {
      notificationView: null
   }

   $(top).on("bulletin_board_changed.alert_widget", function (_, bulletin) {
      if (App.stonehearth.bulletinBoard.view) {
         App.stonehearth.bulletinBoard.view.destroy();
      }

      App.stonehearth.bulletinBoard.view = App.gameView.addView(App.StonehearthBulletinNotification, { context: bulletin });
   });
});

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
         var context = self.get('context');
         var detailViewName = context.config.ui_view;
         self._detailView = App.gameView.addView(App[detailViewName], { context: context })
         self.destroy();
      });
   }
});
