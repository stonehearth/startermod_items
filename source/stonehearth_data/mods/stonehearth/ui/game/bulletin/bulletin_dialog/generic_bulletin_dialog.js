App.StonehearthGenericBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'genericBulletinDialog',
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;
      self._super();

      self.$('#okButton').click(function() {
         var bulletin = self.get('context');
         radiant.call_obj(bulletin.callback_instance, bulletin.data.ok_callback);
         self.destroy();
      });

      self.$('#acceptButton').click(function() {
         var bulletin = self.get('context');
         radiant.call_obj(bulletin.callback_instance, bulletin.data.accepted_callback);
         self.destroy();
      });

      self.$('#declineButton').click(function() {
         var bulletin = self.get('context');
         radiant.call_obj(bulletin.callback_instance, bulletin.data.declined_callback);
         self.destroy();
      });
   }
});
