App.StonehearthTaskManagerView = App.View.extend({
   templateName: 'stonehearthTaskManager',

   init: function() {
      this._super();
      var self = this;
      self.set('context', {});

      radiant.call('radiant:game:start_task_manager')
            .progress(function (response) {
               var foo = JSON.stringify(response)
               $.each(response.counters, function(i, counter) {
                  counter.name = counter.name.replace(/ /g, '_');
               });

               self._refresh(response);
            });
   },

   didInsertElement: function () {
      this.bars    = $('#taskManager').find('#meter');
      this.details = $('#taskManager').find('#details');

      this.bars.click(function() {
         $(self.details).toggle();
      })
   },

   _refresh: function(data) {
      var self = this;
      var totalWidth = 300;
      var scale = totalWidth / data.total_time;
      var sum = 0;

      this.bars.css('width', totalWidth);
      this.bars.css('max-width', totalWidth);

      $.each(data.counters, function(i, counter) {
         if(counter.name != 'idle') {
            var bar = self.bars.find('.' + counter.name);

            if (bar.length == 0) {
               bar = $('<div>')
                  .addClass('counter')
                  .addClass(counter.name);

               self.bars.append(bar)
            } 

            var width = counter.time * scale
            bar.css('width', width);
            sum += width;
         }
      });

      //this.bars.find('.idle').css('width', totalWidth - sum);

      this._populateDetails(data);
   },

   _populateDetails: function(data) {
      var self = this;

      $.each(data.counters, function(i, counter) {
         var row = self.details.find('#' + counter.name);

         if (row.length == 0 ) {
            self.details.find('table').append('<tr id=' + counter.name + '><td class="key ' + counter.name + '">&nbsp;&nbsp;<td class=name>' + counter.name + '<td class=time>' + counter.time);
         } else {
            row.find('.time').html(counter.time)
         }
      });
   },

   destroy: function() {
      this._super();
      this.trace.destroy();
   }

});
   