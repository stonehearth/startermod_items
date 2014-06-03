App.StonehearthBulletinDialog = App.View.extend({
	templateName: 'bulletinDialog',
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;
      self._super();

      self.$('.title .closeButton').click(function() {
         self.destroy();
      });

      self.$('#showMeButton').click(function() {
         var context = self.get('context');
         var entity = context.data.show_me_entity;
         radiant.call('stonehearth:camera_look_at_entity', entity);
         self.destroy();
      });

      self.$('#acceptButton').click(function() {
         var context = self.get('context');
         radiant.call_obj(context.callback_instance, context.config.accepted_callback);
         self.destroy();
      });

      self.$('#declineButton').click(function() {
         var context = self.get('context');
         radiant.call_obj(context.callback_instance, context.config.declined_callback);
         self.destroy();
      });
   },

   willDestroyElement: function() {
      App.bulletinBoard.onDialogViewDestroyed();
      this._super();
   }
});
