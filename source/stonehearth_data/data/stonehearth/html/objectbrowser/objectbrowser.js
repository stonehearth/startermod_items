App.ObjectBrowserView = App.View.extend({
   templateName: 'objectbrowser',
   objectHtml: "",
   loading: false,

   didInsertElement: function() {
      var self = this;

      $(top).on("radiant.events.selection_changed", function (_, evt) {
         var uri = evt.data.selected_entity;
         if (uri) {
            self.fetch(uri);
         }
      });

      
      $("#objectJson").on("click", "a", function(event) {
         event.preventDefault();
         var uri = $(this).attr('href');
         self.fetch(uri);      
      });
   },

   formatJson: function(json) {
      json = JSON.stringify(json, undefined, 2);
      return json.replace(/&/g, '&amp;')
                 .replace(/</g, '&lt;')
                 .replace(/>/g, '&gt;')
                 .replace(/ /g, '&nbsp;')
                 .replace(/\n/g, '<br>')
                 .replace(/(\/[^"]*)/g, '<a href="$1">$1</a>')
                 .replace(/"(mod:\/\/[^"]*)"/g, '<a href="$1">$1</a>')
   },

   fetch: function(uri) {
      var self = this;
      self.set('loading', true);

      $.get(uri)
         .done(function(json) {
            self.set('loading', false);
            self.set('objectHtml', self.formatJson(json));
         });
   }
});

