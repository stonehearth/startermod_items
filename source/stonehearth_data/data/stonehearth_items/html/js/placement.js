$(document).ready(function(){
   $(top).on("place_item.stonehearth_items", function (_, e) {
      $(top).trigger('show_tip.radiant', {
         title : i18n.t('stonehearth_items:place_title') + " " + e.event_data.item_name,
         description : i18n.t('stonehearth_items:place_description')
      });

      // kick off a request to the client to show the cursor for placing
      // the workshop. The UI is out of the 'create workshop' process after
      // this. All the work is done in the client and server
      //

      var placementModUri = '/modules/client/stonehearth_items/place_item';
      $.get(placementModUri, {proxy_entity_id: e.event_data.entity_id,
                              target_mod : e.event_data.target_mod,
                              target_name : e.event_data.target_name
                             })
         .done(function(o){
         })
         .always(function(o) {
            $(top).trigger('hide_tip.radiant');
         });
   });
});