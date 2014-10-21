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

      var cssClass = self.get('context.data.cssClass');
      
      if (cssClass) {
         self.$('.bulletinDialog').addClass(cssClass);
      }
   },

   _triggerCallbackAndDestroy: function(callback_key) {
      var self = this;
      var bulletin = self.get('context');
      var instance = bulletin.callback_instance;
      var method = bulletin.data[callback_key];

      radiant.call_obj(instance, method)
         .done(function(response) {
            if (response.trigger_event) {
               $(top).trigger(response.trigger_event.event_name, response.trigger_event.event_data);
            }
         });
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
