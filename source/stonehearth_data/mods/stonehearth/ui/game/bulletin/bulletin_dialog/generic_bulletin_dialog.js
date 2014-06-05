App.StonehearthGenericBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'genericBulletinDialog',
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

   _triggerCallbackAndDestroy: function(instance, callback_key) {
      var self = this;
      var bulletin = self.get('context');
      radiant.call_obj(bulletin.callback_instance, bulletin.data[callback_key]);
      App.bulletinBoard.markBulletinHandled(bulletin);
      self.destroy();
   }
});
