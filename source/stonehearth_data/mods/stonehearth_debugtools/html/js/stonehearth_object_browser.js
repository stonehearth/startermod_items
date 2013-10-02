App.StonehearthObjectBrowserView = App.View.extend({
   templateName: 'objectBrowser',
   objectHtml: "",
   loading: false,
   history: [],
   forwardHistory: [],

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      $(top).on("selection_changed.radiant", function (_, data) {
         var uri = data.selected_entity;
         if (uri) {
            self.fetch(uri);
         }
      });

      
      $("#body").on("click", "a", function(event) {
         event.preventDefault();
         var uri = $(this).attr('href');
         self.fetch(uri);      
      });

      $('#uriInput').keypress(function(e) {
        if(e.which == 13) {
            $(this).blur();
            self.fetch($(this).val());
        }
    });

      this.goHome();
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
      self.history.push(uri);
      
      if (self.trace) {
         self.trace.destroy();
      }

      self.trace = radiant.trace(uri)
         .progress(function(json) {
            self.set('context.loading', false);
            self.set('context.objectHtml', self.formatJson(json));
            self.set('context.uri', uri);
         })
         .fail(function(e) {
            console.log(e);
         });
   },

   collapse: function() {
      $("#body").toggle();
   },

   goBack: function() {
      if(this.history.length > 1) {
        this.forwardHistory.push(this.history.pop())
      }
      this.fetch(this.history.pop());
      console.log(this.history.length);
   },

   goForward: function () {
      if(this.forwardHistory.length > 0) {
         var uri = this.forwardHistory.pop();
         this.fetch(uri);
      }
      console.log(this.forwardHistory.length);
   },

   goHome: function() {
      this.fetch('/o/stores/game/objects/1');
   }

});

