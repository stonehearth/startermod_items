App.StonehearthErrorBrowserView = App.View.extend({
   templateName: 'errorBrowser',
   components: {
      'entries': []
   },

   init: function() {
      this._super();
      this._traces = {};
      this._results = {};     
   },


   _compileData: function() {
      var self = this;
      var data = {
         total: 0,
         entries: [],
      }
      radiant.each(self._results, function(_, result) {
         radiant.each(result.entries, function(_, entry) {
            data.total += 1;
         });
         data.entries = data.entries.concat(result.entries);
      });
      var current_index = self.get('context.current_index')
      if (current_index == undefined) {
         current_index = 1;
         self.set('context.current_index', 1)
      }
      this.set('context.errors', data);
      this.set('context.current', data.entries[current_index-1])
      if (data.total > 0 && !self._keepHidden) {
         this.$().show();
      }
   },

   _installTrace: function(name, object) {
      var self = this;

      if (this._traces[name]) {
         this._traces[name].destroy();
      }
      var trace = new RadiantTrace();
      trace.traceUri(object, { 'entries' : [] })
         .progress(function(data) {
            self._results[name] = data;
            self._compileData();
         });
      this._traces[name] = trace;
   },

   didInsertElement: function() {
      this.$().hide();
      
      var self = this;
      radiant.call('radiant:client:get_error_browser')
         .done(function(obj) {
            self._installTrace('client_errors', obj.error_browser);
         })
      radiant.call('radiant:server:get_error_browser')
         .done(function (obj) {
            self._installTrace('server_errors', obj.error_browser);
         })

      var self = this;

      this.$().draggable();

      this.$('#entries').on('click', 'a', function(event) {
         var error_uri = $(this).attr('error_uri');
         if (self.currentView) {
            self.currentView.destroy();
         }
         self.currentView = App.gameView.addView(App.StonehearthErrorDialogView, { uri : error_uri });
      });

      this.$('.close').click(function() {
         self.$().hide();
      });

      this.$('#closeForever').click(function() {
         self._keepHidden = true;
         self.$().hide();
      });

      this.$('#next').click(function() {
         var current_index = self.get('context.current_index');
         var data = self.get('context.errors');
         if (current_index < data.total) {
            current_index = current_index + 1
         }
         self.set('context.current_index', current_index)
         self.set('context.current', data.entries[current_index-1])
      });

      this.$('#prev').click(function() {
         var current_index = self.get('context.current_index');
         var data = self.get('context.errors');
         if (current_index > 1) {
            current_index = current_index - 1
         }
         self.set('context.current_index', current_index)
         self.set('context.current', data.entries[current_index-1])
      });
   }

});
