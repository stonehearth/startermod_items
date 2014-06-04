App.StonehearthBaseBulletinDialog = App.View.extend({
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;
      self._super();

      self.$('.title .closeButton').click(function() {
         self.destroy();
      });
   },

   willDestroyElement: function() {
      var self = this;
      var bulletin = self.get('context');
      App.bulletinBoard.onDialogViewDestroyed(bulletin);
      this._super();
   }
});
