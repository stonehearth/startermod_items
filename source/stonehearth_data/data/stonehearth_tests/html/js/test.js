var format_json = function(json) {
   json = JSON.stringify(json, undefined, 2);
   return json.replace(/&/g, '&amp;')
              .replace(/</g, '&lt;')
              .replace(/>/g, '&gt;')
              .replace(/ /g, '&nbsp;')
              .replace(/\n/g, '<br>')
              .replace(/(\/[^"]*)/g, '<a href="$1">$1</a>')
              .replace(/"(mod:\/\/[^"]*)"/g, '<a href="$1">$1</a>')
}

$(document).ready(function() {
   radiant.events.poll();

   var fetch = function(uri) {
      $("#selected_json").html('fetching entity ' + uri + '...');

      $.get(uri)
         .done(function(json) {
            $("#selected_json").html(format_json(json));
         });
   }
   $("#selected_json").on("click", "a", function(event) {
      event.preventDefault();
      var uri = $(this).attr('href');
      fetch(uri);      
   });

   $(top).on("radiant.events.selection_changed", function (_, evt) {
      var uri = evt.data.selected_entity;
      if (uri) {
         fetch(uri);
      }
   });
})