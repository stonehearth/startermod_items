$(document).ready(function(){
   /* keep this around for now in case we want to bring it back
   $(top).on('show_character_sheet.stonehearth', function (_, e) {
      console.error('trigger of o')

      var trace = new RadiantTrace()
      var viewName = 'StonehearthSimpleCharacterSheetView';

      trace.traceUri(e.entity, { unit_info: {} })
         .progress(function(o) {
            if (trace) {
              trace.destroy();
              trace = null;
            }

            var uri = o.unit_info.character_sheet_info;

            $.get(uri)
               .done(function(data) {
                  if (data.view_name) {
                     viewName = data.view_name;
                  }
                  App.gameView.addView(App[viewName], { uri: e.entity, character_sheet_info: data });
               });
         })
         .fail(function(e) {
            console.error(e);
            trace.destroy();
            trace = null;
         });
      
      if (App.stonehearthCharacterSheet != null) {
         App.stonehearthCharacterSheet.destroy();
         App.stonehearthCharacterSheet = null;
      }
      App.stonehearthCharacterSheet = App.gameView.addView(App.StonehearthCitizenCharacterSheetView, { uri: e.entity });
      */
});

