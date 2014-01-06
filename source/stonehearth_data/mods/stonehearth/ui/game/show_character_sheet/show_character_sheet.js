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
      return { date: "date", title: "title", text: "this iss the entry" }
   }

});