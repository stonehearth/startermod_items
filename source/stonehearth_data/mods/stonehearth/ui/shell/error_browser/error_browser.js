App.StonehearthErrorBrowserView = App.View.extend({
   templateName: 'errorBrowserFrame',
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
         total: 12,
         entries: [],
      }
      $.each(self._results, function(_, result) {
         data.entries = data.entries.concat(result.entries);
      });
      this.set('context.errors', data);
   },

   _installTrace: function(name, object) {
      var self = this;

      if (this._trace[name]) {
         this._trace[name].destroy();
      }
      var trace = new RadiantTrace();
      trace.traceUri(object, { 'entries' : [] })
         .progress(function(data) {
            self._results[name] = data;
            self._compileData();
         });
      this._trace[name] = trace;
   },

   didInsertElement: function() {
      var self = this;
      console.log('inserted error browser');

      radiant.call('radiant:client:get_error_browser')
         .done(function(obj) {
            self._installTrace('client_errors',  obj.error_browser)
         })
      radiant.call('radiant:server:get_error_browser')
         .done(function(obj) {
            self._installTrace('server_errors',  obj.error_browser)
         })

      var self = this;
      $('#errorBrowser').draggable();
      $('#errorBrowser').find('#entries').on('click', 'a', function(event) {
         var error_uri = $(this).attr('error_uri');
         if (self.currentView) {
            self.currentView.destroy();
         }
         self.currentView = App.gameView.addView(App.StonehearthErrorDialogView, { uri : error_uri });
      });
   },

   actions: {
      toggleEntries: function () {
         $('#errorBrowser').find('#entries').toggle();
      }
   }
});
