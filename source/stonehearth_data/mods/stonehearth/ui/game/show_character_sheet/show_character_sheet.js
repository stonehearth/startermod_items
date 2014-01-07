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
      "stonehearth:personality" : {}
   },

   firstJournalEntry: function() {
      var log = this.get('stonehearth:personality.log');

      if (log && log.length > 0) {
         return log[1];
      } else {
         return { title: "no entries" };
      }
   }.property()

});