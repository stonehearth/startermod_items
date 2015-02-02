App.StonehearthBaseBulletinDialog = App.View.extend({
   closeOnEsc: true,
   uriProperty: 'model',

   didInsertElement: function() {
      var self = this;
      self._super();

      self.dialog = self.$('.bulletinDialog');

      this._wireButtonToCallback('#okButton',      'ok_callback');
      this._wireButtonToCallback('#nextButton',    'next_callback', true);    // Keep the dialog around on next
      this._wireButtonToCallback('#acceptButton',  'accepted_callback');
      this._wireButtonToCallback('#declineButton', 'declined_callback');

      var cssClass = self.get('model.data.cssClass');     
      if (cssClass) {
         self.dialog.addClass(cssClass);
      }
   },

   // if the ui_view value changes while we're up, ask App.bulletinBoard
   // to re-create a new view and destory us when that view becomes visible.
   _checkView : function() {
      var self = this;
      var bulletin = self.get('model')
      var viewClassName = String(self.constructor);
      if ('App.' + bulletin.ui_view != viewClassName) {
         // set a flag to prevent calling back into the bulletin board
         // on destroy.  Otherwise, the board gets confused when it tries
         // to make sure it creates the new view for this bulletin before
         // this one has a chance to die (see recreateDialogVIew)
         self._dontNotifyDestroy = true;
         App.bulletinBoard.recreateDialogView(bulletin);
      }
   }.observes('model.ui_view'),

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

   _wireButtonToCallback: function(buttonid, callback, keepAround) {
      var self = this
      self.dialog.on('click', buttonid, function() {
         self._callCallback(callback);
         if (!keepAround) {
            self._autoDestroy();
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
      if (!self._dontNotifyDestroy) {
         var bulletin = self.get('model'); 
         App.bulletinBoard.onDialogViewDestroyed(bulletin);
      }
      this._super();
   }
});
