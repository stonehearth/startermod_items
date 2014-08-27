var RadiantConsole;

(function () {
   // Convert a JSON object so some printable HTML
   var jsonToHTML = function(json) {
      var str = JSON.stringify(json, undefined, 2);
      return str.replace(/&/g, '&amp;')
                .replace(/</g, '&lt;')
                .replace(/>/g, '&gt;')
                .replace(/ /g, '&nbsp;')
                .replace(/\n/g, '<br>')
   }

   // The Command class represents a single running command.  Commands render
   // themselves into a <div></div> and live as long as necessary (some may never
   // complete until explicitly closed).
   var Command = SimpleClass.extend({
      init: function(console, options, cmdline) {
         var self = this;
         var cmd = cmdline.split(' ')[0];
         var args = cmdline.split(" ").slice(1);

         this._options = options;

         // Create a new view for this command.  The content for this view can be
         // retrieved from the command callback via the 'getContainer method'
         this._view = console.createView(cmdline);

         // Call the command implementation...
         var deferred = this._options.call(this, cmd, args);
         if (deferred) {
            // the caller returned a deferred!  wire it up to automatically
            // show the progress of the call
            deferred.progress(function(o) {
               self.setContent(jsonToHTML(o));
            });
            deferred.done(function(o) {
               self.setContent(jsonToHTML(o));
            });
            deferred.fail(function(o) {
               self.setContent(jsonToHTML(o));
            });
         }
      },

      getContainer: function() {
         return this._view.find('.content')
      },

      setContent: function(content) {
         this.getContainer().html(content);
      }
   });

   // The console...
   var Console = SimpleClass.extend({
      init: function() {
         this._commands = {};
         this._running_commands = [];
      },

      register : function(cmd, options) {
         this._commands[cmd] = options
      },

      setContainer : function(container) {
         this._container = container
      },

      createView : function(title) {
         var view = $('<div><div class="title">' + title + '</div><div class="content"></div></div>');
         this._container.append(view);
         return view
      },

      run : function(cmdline) {
         var cmd = cmdline.split(' ')[0];
         var options = this._commands[cmd]
         var command = new Command(this, options, cmdline);
         this._running_commands.push(command);
      }
   });

   radiant.console = new Console();
})();

   