App.StonehearthSimpleCharacterSheetView = App.View.extend({
	templateName: 'simpleCharacterSheet',
   modal: true,

   components: {
      "unit_info": {}
   },

   didInsertElement: function() {
      console.log(this.get('character_sheet_info'));      
   }

});