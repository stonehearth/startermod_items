$(document).ready(function(){
   $(top).on('show_journal.stonehearth', function (_, e) {
      var view = App.gameView.addView(App.StonehearthJournalView, { uri: e.entity });
   });
});

App.StonehearthJournalView = App.View.extend({
   templateName: 'stonehearthJournal',
   components: {
      'unit_info' : {},
      'stonehearth:personality' : {}
   },
   modal: true,

   didInsertElement: function() {
      this._super();
      if (this.get('context.stonehearth:personality') == undefined ) {
         return;
      }

      $('#journalWindow').animate({ bottom: 100 }, {duration: 500, easing: 'easeOutBounce'});
   }

});