App.StonehearthBulletinList = App.View.extend({
	templateName: 'bulletinList',
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;
      self._super();

      this.$().on('click', '.row', function() {
         var row = $(this);
         var id = row.attr('id');
         App.bulletinBoard.showDialogViewForId(id);
      });

      this.$('.title .closeButton').click(function() {
         self.destroy();
      });

   },
});

App.StonehearthBulletinListWidget = App.View.extend({
   templateName: 'bulletinListWidget',

   didInsertElement: function() {
      var self = this;
      self._super();

      self.$('#bulletinListWidget').click(function() {
         App.bulletinBoard.showListView();
      });
   },
});
