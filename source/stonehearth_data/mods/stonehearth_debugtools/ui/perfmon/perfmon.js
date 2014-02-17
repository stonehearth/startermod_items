$(document).on('stonehearthReady', function(){
   App.debugDock.addToDock(App.StonehearthPerfmonIcon);
});

App.StonehearthPerfmonIcon = App.View.extend({
   templateName: 'perfmonIcon',
   classNames: ['debugDockIcon'],

   didInsertElement: function() {
      this.$().click(function() {
         App.debugView.addView(App.StonehearthPerfmonView)
      })
   }
});

App.StonehearthPerfmonView = App.View.extend({
   templateName: 'Perfmon',
   objectHtml: "",
   loading: false,
   history: [],
   forwardHistory: [],

   init: function() {
      this._suffixTypes = {
         'memory' : {
            base : 1024.0,
            suffix : [ 'B', 'KB', 'MB', 'GB', 'TB', 'PB' ],
         },
         default : {
            base : 1000.0,
            suffix : [ '', 'K', 'M', 'G', 'T', 'P' ],
         }
      };
      this._perf_trace = new RadiantTrace();
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      this.$().draggable();

      this.my('.close').click(function() {
         self.destroy();
      });

      self._perf_trace = radiant.call('radiant:game:get_perf_counters')
         .progress(function(data) {
            self._updateCounters(data);
         });
   },

   destroy: function() {
      if (this._perf_trace) {
         // XXX, I would really like to destroy this, but _perf_trace apparently
         // doesn't have a destroy method, at least that's what the chrome debugger claims
         //this._perf_trace.destroy();
         this._perf_trace = null;
      }
      this._super();
   },

   _formatCounter: function(d) {
      var s = this._suffixTypes[d.type]
      if (!s) {
         s = this._suffixTypes.default;
      }
      suffix_index = 0;
      var count = d.value
      while (count > s.base) {
         count = count / s.base;
         suffix_index += 1;
      }
      if (suffix_index != 0) {
         count = count.toFixed(2)
      }
      return count + ' ' + s.suffix[suffix_index]
   },

   _updateCounters: function (data) {
      // this should not be necessary, but the trace is not being destroyed properly
      if (!this._perf_trace) {
         return
      }

      var self = this;
      var counters = [];

      $.each(data, function(i, d) {
         var entry = {
            name : d.name,
            value : self._formatCounter(d)
         };
         counters.push(entry)
      })

      counters.sort( function(l, r) { return l.name < r.name })
      this.set('context.counters', counters)
   },

});

