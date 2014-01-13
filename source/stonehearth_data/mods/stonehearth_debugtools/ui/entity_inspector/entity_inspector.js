App.StonehearthAiExecutionFrame = App.View.extend({
   templateName: 'executionFrame',
})

App.StonehearthAiExecutionUnit = App.View.extend({
   templateName: 'executionUnit',
})

App.StonehearthEntityInspectorView = App.View.extend({
   templateName: 'entityInspector',

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      self.set('context.state', 'no entity selected');
      $(top).on("radiant_selection_changed.entity_inspector", function (_, data) {
         var uri = data.selected_entity;
         if (uri) {
            self.fetch(uri);
         }
      });

      /*
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
      this.collapse();
      */
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

   fetch: function(entity) {
      var self = this;
      self.set('context.state', 'fetching ai component uri');
      
      if (self.trace) {
         self.trace.destroy();
         self.trace = null;
      }
      self.trace = radiant.trace(entity);
      self.trace
         .progress(function(json) {
            if (self.trace) {
              self.trace.destroy();
              self.trace = null;
            }
            self.set('context.state', 'updating')
            self._aiComponent = json['stonehearth:ai']
            self.fetchAiComponent()
         })
         .fail(function(e) {
            console.log(e);
         });
   },

   fetchAiComponent: function() {
      var self = this;
      self.set('context.state', 'updating...');
      radiant.call_obj(self._aiComponent, 'get_debug_info')
        .done(function(debugInfo) {
           self.set('context.state', 'updating..');
           self.set('context.ai', debugInfo)
           if (self._aiComponent) {
              self.set('context.state', 'updating. ' + new Date().getTime());
              self._fetchTimer = setTimeout(function() {
                self.fetchAiComponent()
              }, 500);
              self.set('context.state', 'updating');
           } else {
              self.set('context.state', 'idle.');
           }
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

