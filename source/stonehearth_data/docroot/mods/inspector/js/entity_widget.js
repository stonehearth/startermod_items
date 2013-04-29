var EntityWidget = Class.extend({
   init: function () {
      var self = this;
      this.entityId = null;

      this.container = $("#entity_widget_container");
      this.view = $('#templates > #entity_widget').clone();


      this.container.prepend(this.view);

      // initialize the plot after adding to the DOM.  Otherwise, the plot
      // library will get angry
      var plot_area = this.view.find("#intention_plot");
      this.plot_width = plot_area.width()
      this.intention_plot = $.plot(plot_area, [], {
         xaxis: { show: false }
      });
      this.setEntityId(0);
   },

   setEntityId: function (id) {
      this._entity_id = id;

      var self = this;
      var title = id ? ('Entity ' + id) : 'No entity';
      this.view.find("#title").html(title);
      this.view.find('#thread_stack').html("");

      if (this.ai_websocket) {
         this.ai_websocket.send('close')
         this.ai_websocket.close();
         this.ai_websocket = null
      }
      if (id) {
         this.activity_names = {};
         this.intention_data = [];
         this.intention_plot_timer = setInterval(function() {
            self._update_plot();
         }, 50);

         if (this.trace) {
            this.trace.destroy();
         }
         this.trace = radiant.trace_entity(id)
            .filter('identity')
            .progress(function(data) { self.update_components(data); });
         this._connectDebugWebSocket(id);
      }
   },

   _connectDebugWebSocket: function(id) {
      var self = this;
      var url = 'ws://' + window.location.hostname + ':' + 13337;

      this.ai_websocket = new WebSocket(url, "game-ai");
      console.log('connecting web socket to ' + url)
      this.ai_websocket.onmessage = function(ev) {
         var info = JSON.parse(ev.data);
         if (info.entity_id == self._entity_id) {
            if (info.activities) {
               self.update_ai(info.activities);
            }
            var stack = info.stack || "no stack";
            self.view.find('#thread_stack').html(stack);
            self.ai_websocket.send('fetch')
         }
      }
      this.ai_websocket.onopen = function(ev) {
         self.ai_websocket.send(id.toString());
      }
      this.ai_websocket.onerror = function(error) {
         console.log('websocket error')
      }
      this.ai_websocket.onclose = function() {
         console.log('websocket closed')
      }
   },

   update_components: function(data)
   {
      var info = JSON.stringify(data, null, 2);
      this.view.find("#info").html(info);
   },

   update_ai: function (info)
   {
      var self = this;
      if (this.intention_data.length > this.plot_width) {
         this.intention_data = this.intention_data.slice(1)
      }
      var next_data_entry = {};
      $.each(info.priorities, function(intention_name, value) {
         var activity_name = value.activity[0];
         self.activity_names[activity_name] = true;
         next_data_entry[activity_name] = value.priority;
      });
      this.intention_data.push(next_data_entry);
   },

   _update_plot: function() {
      var idata = this.intention_data;
      var plot_data = [];

      $.each(this.activity_names, function(activity_name, _) {
         var entry = {
            label: activity_name,
            data: []
         };
         for (var i = 0; i < idata.length; i++) {
            var pri = idata[i][activity_name] || 0;
            entry.data.push([i, pri]);
         }
         plot_data.push(entry);
      });
      this.intention_plot.setData(plot_data);
      this.intention_plot.setupGrid();
      this.intention_plot.draw();

   }
});
