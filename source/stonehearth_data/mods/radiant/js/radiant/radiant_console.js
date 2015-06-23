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
      init: function(console, id, options, cmdline) {
         var self = this;
         var cmd = cmdline.split(' ')[0];
         var args = cmdline.split(" ").slice(1);

         this._cmdid = id;
         this._options = options;

         // Create a new view for this command.  The content for this view can be
         // retrieved from the command callback via the 'getContainer method'
         this._view = console.createView(cmdline, id);
         this.setViewMode('raw');

         if (!this._options) {
            self.setContent("unknown command");
            return;
         }

         // Call the command implementation...
         var deferred = this._options.call(this, cmd, args);
         if (deferred) {
            // the caller returned a deferred!  wire it up to automatically
            // show the progress of the call
            deferred.progress(function(o) {                        
               if (self._viewMode == 'raw') {
                  self.setContent(jsonToHTML(o));
               } else if (self._viewMode == 'graph') {
                  self._updateGraphSeries(o, options.graphValueKey);
               }
            });
            deferred.done(function(o) {
               if (self._viewMode == 'raw') {
                  self.setContent(jsonToHTML(o));
               }
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
      },

      getViewMode: function() {
         return this._viewMode;
      },

      setViewMode: function(mode) {
         var self = this;

         self._viewMode = mode;
         if (mode == 'graph') {
            self._currentX = 0;
            self._graphSeries = [];
            self._graphSeriesData = {}
            self._graphPalette = new Rickshaw.Color.Palette( { scheme: 'classic9' } );
            self._graphPalette.color(); // skip dark purple!
            var content = '\
            <div class="graph"> \
               <div id="legend_' + self._cmdId + '" class="legend"></div> \
               <div class="chart_container" class="rickshaw_legend"> \
                  <div class="yaxis" id="yaxis_' + self._cmdid + '">.</div> \
                  <div class="chart" id="chart_' + self._cmdid + '"></div> \
               </div> \
            </div>';
            self.setContent(content);
         } else {
            self._graph = undefined;
            self._graphSeries = undefined;
            self._graphSeriesData = undefined;
            self.setContent("");
         }
      },

      _terminate : function() {
         if (this._options && this._options.terminate) {
            this._options.terminate();
         }
      },

      _updateGraphSeries : function (o, valueKey) {
         var self = this;
         var series = self._graphSeries;
         var seriesData = self._graphSeriesData;
         var minX = self._currentX - 10;

         valueKey = valueKey != undefined ? valueKey : "value";

         $.each(o, function(k, v) {
            var o = seriesData[k];
            if (!o) {
               o = {
                  color : self._graphPalette.color(),
                  data  : [],
                  name  : k,
               };
               seriesData[k] = o;
               series.push(o);
            }
            while (o.data.length > 0 && o.data[0].x < minX) {
               o.data.shift();
            }

            var value;
            if (typeof v == 'number') {
               value = v;
            } else if (v instanceof Object && typeof v[valueKey] == 'number') {
               value = v[valueKey];
            }
            o.data.push({
               x : self._currentX,
               y : value,
            })
         });
         self._currentX = self._currentX + 1;
         self._updateGraph();
      },

      _updateGraph : function () {
         var self = this;
         if (!$.isEmptyObject(self._graphSeries)) {
            if (!self._graph) {
               self._graph = new Rickshaw.Graph( {
                     element: document.getElementById('chart_' + self._cmdid),
                     width: 400,
                     height: 80,                     
                     renderer: self._options.graphType || 'stack',
                     series: self._graphSeries,
                  } );
               self._graphLegend = new Rickshaw.Graph.Legend( {
                     graph: self._graph,
                     element: document.getElementById('legend_' + self._cmdId)
                  } );
               self._graphYTicks = new Rickshaw.Graph.Axis.Y( {
                     graph: self._graph,
                     orientation: 'left',
                     pixelsPerTick: 20,
                     tickFormat: Rickshaw.Fixtures.Number.formatKMBT,
                     element: document.getElementById('yaxis_' + self._cmdId)
               } );               
            } else {
               self._graph.validateSeries(self._graphSeries);
               self._graph.update();
            }
         }
      }
   });

   // The console...
   var Console = SimpleClass.extend({
      init: function() {
         this._nextCommandId = 1;
         this._commands = {};
         this._running_commands = [];
      },

      register : function(cmd, options) {
         this._commands[cmd] = options
      },

      setContainer : function(container) {
         var self = this;
         this._container = container
         container.on('mouseenter', '.command', function() {
            $(this).addClass('hover')
         });
         container.on('mouseleave', '.command', function() {
            $(this).removeClass('hover')
         });
         container.on('mouseenter', '.button', function() {
            $(this).addClass('hover')
         });
         container.on('mouseleave', '.button', function() {
            $(this).removeClass('hover')
         });
         container.on('click', '#popout_button', function() {
            var cmdid = $(this).parents('.command').attr('cmdid');
            var command = self._container.find('.command[cmdid="' + cmdid +'"]');
            if (command.hasClass('floating')) { 
               command.removeClass('floating')
                      .css({
                         top:  0,
                         left: 0,
                      })
                      //.draggable("disable")
            } else {
               var offset = command.position();
               command.addClass('floating')
                      .css({
                         top:  offset.top + 10,
                         left: offset.left + 10,
                      })
                      .draggable();
            }
         });
         container.on('click', '#close_button', function() {
            var cmdid = $(this).parents('.command').attr('cmdid');
            if (self._running_commands[cmdid]) {
               self._running_commands[cmdid]._terminate();
               delete self._running_commands[cmdid];
               self._container.find('.command[cmdid="' + cmdid +'"]').remove();
            }
         });
         container.on('click', '#graph_button', function() {
            var cmdid = $(this).parents('.command').attr('cmdid');
            var command = self._running_commands[cmdid];
            if (command) {
               var viewMode = command.getViewMode();
               if (viewMode == 'raw') {
                  viewMode = 'graph';
               } else {
                  viewMode = 'raw';
               }
               command.setViewMode(viewMode);
            }
         });
      },

      createView : function(title, cmdid, options) {
         var self = this;

         var buttons = '<div class="buttons"> \
                           <div id="graph_button" class="button"></div> \
                           <div id="popout_button" class="button"></div> \
                           <div id="close_button" class="button"></div> \
                        </div>';
         var view = $('<div class="command" cmdid="'+ cmdid + '"><div class="title">' + title + '</div>' + buttons + '<div class="content"></div></div>');
         this._container.append(view);
         setTimeout(function() { self._container.scrollTop(1E10); }, 100);
         
         return view
      },

      run : function(cmdline) {
         var cmd = cmdline.split(' ')[0];
         var options = this._commands[cmd]
         var cmdid = this._nextCommandId++;

         this._running_commands[cmdid] = new Command(this, cmdid, options, cmdline);
      },

      getCommands : function() {
         // Print out all available console commands
         var self = this;
         var commandsString = "";
         radiant.each(this._commands, function(key, value) {
            commandsString = commandsString + "<p>" +  key + ' - ' + value.description + '</p>';
         });

         return commandsString;
      }

   });

   radiant.console = new Console();
})();

   