App.StonehearthResourceScoreboardView = App.View.extend({
   templateName: 'resourceScoreboard',

   components: {
      'resource_types' : {

      }
   },

   init: function() {
      this._super();
      var self = this;
      radiant.call('stonehearth:get_resource_tracker')
         .done(function(response) {
            console.log(response.tracker);
            self.trace = radiant.trace(response.tracker)
               .progress(function(data) {
                  self.update(data);
               })
               .fail(function(e) {
                  console.log(e);
               });

            
            //self.set('uri', response.tracker);
         });
   },

   update: function(data) {
      var newData = data.resource_types;
      var board = $('#resourceScoreboard')

      for (var entry in newData) {

         if (newData.hasOwnProperty(entry)) {
            item = newData[entry]
            id = entry.replace(/:/g, "_");
            var itemDiv = board.find('#' + id)

            if (itemDiv.length == 0) {
               var row = $('<div>')
                  .addClass('row');

               $('<img>')
                  .attr('src', item.icon)
                  .addClass('icon')
                  .appendTo(row);

               value = $('<div>')
                  .attr('id', id)
                  .addClass('value')
                  .html(item.count)
                  .appendTo(row);

               board.append(row)
            } else {
               itemDiv.html(item.count);
            }
         }
      }
   },

   foo: function() {

      var ctx = this.get('context');
      var newData = this.get('context').resource_types

      /*
      var board = $('#resourceScoreboard')

      for (var entry in newData) {

         if (newData.hasOwnProperty(entry)) {
            item = newData[entry]
            id = entry.replace(/:/g, "_");
            var itemDiv = board.find('#' + id)

            if (itemDiv.length == 0) {
               board.append('<div>--' + item.count + '</div>');
            } else {
               itemDiv.html(item.count);
            }
         }
      }
      */

      var resources = []
      for (var entry in newData) {
          if (newData.hasOwnProperty(entry)) {
            resources.push({
               name: entry,
               count: newData[entry]
            })
          }
      }
      //this.set('context.resources', resources);
   }.observes('context'),

});

