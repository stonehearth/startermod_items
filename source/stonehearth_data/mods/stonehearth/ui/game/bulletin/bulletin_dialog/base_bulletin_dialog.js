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
