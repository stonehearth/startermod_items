$(document).ready(function() {
   //App.gameView.addView(App.ThunderdomeActionController);
});

App.ThunderdomeActionController = App.View.extend({
   templateName: 'actionController',

   components: {
      "stonehearth:commands": {
         commands: []
      },
      "stonehearth:buffs" : {},
      "unit_info": {}
   },

   init: function() {
      this._super();
      var self = this;

      $.get('/thunderdome/data/actions.json')
         .done(function(json) {
            self.set('context', json);
         });

      var self = this;
      $(top).on("radiant_selection_changed.action_controller", function (_, data) {
        self._selected = data.selected_entity;
        /*
         if (!self._supressSelection) {
           var selected = data.selected_entity;
           self._selected_entity = selected;
           if (self._selected_entity) {
              self.set('uri', self._selected_entity);
              self.show();
           } else {
              //self.set('uri', null);
              self.hide();
           }
         }
         */
      });
   },

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {

   },

   actions: {
      
      doAction: function (action, response, playerNum) {
         var self = this;
         console.log(String(action.effect));
         radiant.call('thunderdome:do_action', playerNum, action.name, action.reactions[response] );
      }
   },

    _actionsArray: function() {
        var vals = [];
        var attributeMap = this.get('context.actions');
        
        if (attributeMap) {
           $.each(attributeMap, function(k ,v) {
              if(k != "__self" && attributeMap.hasOwnProperty(k)) {
                 v.name = k;
                 vals.push(v);
              }
           });
        }

        this.set('context.actionsArray', vals);
    }.observes('context.actions'),   

});
