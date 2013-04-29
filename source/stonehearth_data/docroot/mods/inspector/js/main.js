
var god = {

};

$(function () {
   radiant.api.poll();

   // create a new entity widget for the selected object
   var selected_entity_widget = new EntityWidget();
   $(top).on("radiant.events.selection_changed", function (_, evt) {
      selected_entity_widget.setEntityId(evt.data.entity_id);
   });

});
