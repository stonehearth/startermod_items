$(document).ready(function(){
      $(top).on("stonehearth_crafter.show_craft_ui", function (_, e) {
         console.log('showing crafting ui for entity ' + e.entity)
      });
});