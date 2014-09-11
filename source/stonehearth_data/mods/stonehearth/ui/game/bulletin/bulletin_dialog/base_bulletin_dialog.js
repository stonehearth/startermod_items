App.StonehearthBaseBulletinDialog = App.View.extend({
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;
      self._super();

      self.$('#okButton').click(function() {
         self._triggerCallbackAndDestroy('ok_callback');
      });

      self.$('#acceptButton').click(function() {
         self._triggerCallbackAndDestroy('accepted_callback');
      });

      self.$('#declineButton').click(function() {
         self._triggerCallbackAndDestroy('declined_callback');
      });
   },

   _triggerCallbackAndDestroy: function(callback_key) {
      var self = this;
      var bulletin = self.get('context');
      var instance = bulletin.callback_instance;
      var method = bulletin.data[callback_key];

      radiant.call_obj(instance, method);
      App.bulletinBoard.markBulletinHandled(bulletin);
      self.destroy();
   },

   willDestroyElement: function() {
      var self = this;
      var bulletin = self.get('context');
      App.bulletinBoard.onDialogViewDestroyed(bulletin);
      this._super();
   }
});
