$(document).ready(function(){
   $(top).on('show_character_sheet.stonehearth', function (_, e) {
      //radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:journal_up' )
      var view = App.gameView.addView(App.StonehearthCharacterSheetView, { uri: e.entity });
   });
});

App.StonehearthCharacterSheetView = App.View.extend({
	templateName: 'characterSheet',
   modal: true,

   components: {
      "unit_info": {},
      "stonehearth:buffs" : {},
      "stonehearth:attributes" : {},
      'stonehearth:personality' : {}
   },

   _setFirstJournalEntry: function() {
      var log = this.get('context.stonehearth:personality.log');

      if (log && log.length > 0) {
         this.set('context.firstJournalEntry', log[0]);
      } else {
         this.set('context.firstJournalEntry', { title: "no entries" });
      }

   }.observes('context.stonehearth:personality'),

   didInsertElement: function() {
      var p = this.get('context.stonehearth:personality');

      if (p) {
         $('#personality').html($.t(p.personality));   
      }
      
   }

});