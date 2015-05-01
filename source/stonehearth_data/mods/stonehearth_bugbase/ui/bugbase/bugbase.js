App.BugBaseView = App.View.extend({
   templateName: 'bugBase',

   init: function(o) {
      this._super();
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      this.fileBugView.hide();
      this.$('#bugbase').click(function() {
         if (self.fileBugView.visible()) {
            self.fileBugView.hide();
         } else {
            self.fileBugView.show();
         }
      });
   },
});

App.FileBugView = App.View.extend({
   templateName: 'fileBugNoKey',

   init: function(o) {
      var self = this
      this._traces = {}
      this._super();
      radiant.call('radiant:get_config', 'redmine_api_access_key')
         .done(function(o) {
            if (o.redmine_api_access_key) {
               self.set('redmine_api_access_key', o.redmine_api_access_key);
               self.set('templateName', 'fileBug');
               self.rerender();
            }
         })
      radiant.call('radiant:client:get_error_browser')
         .done(function(obj) {
            self._installTrace('client_errors', obj.error_browser);
         })
      radiant.call('radiant:server:get_error_browser')
         .done(function (obj) {
            self._installTrace('server_errors', obj.error_browser);
         })

   },

   didInsertElement: function() {
      var self = this;

      this.$("#cancel").click(function() {
         self.hide();
      });

      this.$("#submit").click(function() {
         if ($(this).hasClass('disabled')) {
            return;
         }
         var STONEHEARTH_PROJECT_ID = 1;
         var screenshot = self.$("#take_screenshot").is(':checked');
         var include_lua_error = self.$("#include_lua_error").is(':checked');

         var subject = self.$("#summary").val();
         var description = self.$("#description").val();
         if (include_lua_error && self._firstError) {
            description = description + "\n\n";
            description = description + '**Backtrace**\n';
            description = description + '\n' + 
                                        '~~~\n' +
                                        self._firstError.summary + '\n' +
                                        self._firstError.backtrace + '\n'
                                        '~~~\n';
         }

         var issues = {
            key: self.get('redmine_api_access_key'),
            issue: {
               project_id:    STONEHEARTH_PROJECT_ID,
               subject:       subject,
               description:   description,
            }
         }

         self.$('#cancel').addClass('disabled')
         self.$('#submit').addClass('disabled')               
         radiant.call('radiant:post_redmine_issues', issues, { screenshot: screenshot, })
            .done(function(o) {
               console.log(o);
               self.hide();
            })
            .fail(function(o) {
               console.log(o);
               alert(o.errors[0]);
            })
            .always(function(o) {
               self.$('#cancel').removeClass('disabled')
               self.$('#submit').removeClass('disabled')               
            })
      });
   },

   _installTrace: function(name, object) {
      var self = this;

      if (this._traces[name]) {
         this._traces[name].destroy();
      }
      var trace = new RadiantTrace();
      trace.traceUri(object, { 'entries' : [] })
         .progress(function(data) {
            if (!self._firstError) {
               self._firstError = data.entries[0];
            }
         });         
      this._traces[name] = trace;
   },


});

$(document).on('stonehearthReady', function(){
   // look for the start menu...
   var fileBugView = App.gameView.addView(App.FileBugView);
   App.gameView.addView(App.BugBaseView, { fileBugView: fileBugView });

   /*
   var interval, startMenu;
   var patchStartMenu = function() {
      console.log('patching start menu');
   };

   var findStartMenu = function() {
      console.log('looking for start menu');
      startMenu = $('#startMenu');
      if (startMenu) {
         setTimeout(patchStartMenu, 500)
         clearInterval(interval);
      }
   };
   interval = setInterval(findStartMenu, 500);
   */
});
