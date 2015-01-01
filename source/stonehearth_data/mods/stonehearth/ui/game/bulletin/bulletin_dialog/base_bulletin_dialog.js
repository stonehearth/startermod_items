App.StonehearthBaseBulletinDialog = App.View.extend({
   closeOnEsc: true,
   uriProperty: 'model',

   didInsertElement: function() {
      var self = this;
      self._super();

      var dialog = self.$('.bulletinDialog');

      dialog.on('click', '#okButton', function() {
         self._callCallback('ok_callback');
         self._autoDestroy();
      });

      dialog.on('click', '#nextButton', function() {
         self._callCallback('next_callback');
         // Intentionally not calling autoDestroy.
      });

      dialog.on('click', '#acceptButton', function() {
         self._callCallback('accepted_callback');
         self._autoDestroy();
      });

      dialog.on('click', '#declineButton', function() {
         self._callCallback('declined_callback');
         self._autoDestroy();
      });

      var cssClass = self.get('model.data.cssClass');
      
      if (cssClass) {
         self.$('.bulletinDialog').addClass(cssClass);
      }
   },

   _callCallback: function(callback_key) {
      var self = this;
      var bulletin = self.get('model');
      var instance = bulletin.callback_instance;
      var method = bulletin.data[callback_key];

      radiant.call_obj(instance, method)
         .done(function(response) {
            if (response.trigger_event) {
               $(top).trigger(response.trigger_event.event_name, response.trigger_event.event_data);
            }
         });
   },

   _autoDestroy: function() {
      var self = this;
      var bulletin = self.get('model');
      if (!bulletin.keep_open) {
         App.bulletinBoard.markBulletinHandled(bulletin);
         self.destroy();
      }
   },

   willDestroyElement: function() {
      var self = this;
      var bulletin = self.get('model');
      App.bulletinBoard.onDialogViewDestroyed(bulletin);
      this._super();
   }
});
