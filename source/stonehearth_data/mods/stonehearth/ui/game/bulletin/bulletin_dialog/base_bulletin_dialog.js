App.StonehearthBaseBulletinDialog = App.View.extend({
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;
      self._super();
   },

   willDestroyElement: function() {
      var self = this;
      var bulletin = self.get('context');
      App.bulletinBoard.onDialogViewDestroyed(bulletin);
      this._super();
   }
});
