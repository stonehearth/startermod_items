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
      $.each(self._results, function(_, result) {
         $.each(result.entries, function(_, entry) {
            data.total += 1;
         });
         data.entries = data.entries.concat(result.entries);
      });
      this.set('context.errors', data);

      if (data.total > 0) {
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

      this.my('#entries').on('click', 'a', function(event) {
         var error_uri = $(this).attr('error_uri');
         if (self.currentView) {
            self.currentView.destroy();
         }
         self.currentView = App.gameView.addView(App.StonehearthErrorDialogView, { uri : error_uri });
      });

      this.my('.close').click(function() {
         self.$().hide();
      });
   }

});
