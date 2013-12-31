App.StonehearthTaskManagerView = App.View.extend({
   templateName: 'stonehearthTaskManager',

   init: function() {
      this._super();
      var self = this;
      self.set('context', {});

      radiant.call('radiant:game:start_task_manager')
            .progress(function (response) {
               var foo = JSON.stringify(response)
               console.log(foo)
               self._refresh(response)
            })
   },

   didInsertElement: function () {
      this.bars = $('#taskManager');

   },

   _refresh: function(data) {
      var self = this;
      var totalWidth = 400;
      var scale = totalWidth / data.total_time;
      var sum = 0;

      this.bars.css('width', totalWidth);
      
      $.each(data.counters, function(i, counter) {
         if(counter.name != 'idle') {
            var className = counter.name.replace(/ /g, '_');
            var bar = self.bars.find('.' + className);

            if (bar.length == 0) {
               bar = $('<div>')
                  .addClass('counter')
                  .addClass(className);

               self.bars.append(bar)
            } 

            var width = counter.time * scale
            bar.css('width', width);
            sum += width;
         }
      });

      //this.bars.find('.idle').css('width', totalWidth - sum);
   },

   destroy: function() {
      this._super();
      this.trace.destroy();
   }

});
   