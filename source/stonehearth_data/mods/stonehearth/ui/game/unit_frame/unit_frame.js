App.StonehearthUnitFrameView = App.View.extend({
	templateName: 'unitFrame',

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
      $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
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
      });
   },

   supressSelection: function(supress) {
      this._supressSelection = supress;
   },

   buffs: function() {
        var vals = [];
        var attributeMap = this.get('context.stonehearth:buffs');
        
        if (attributeMap) {
           $.each(attributeMap, function(k ,v) {
              if(k != "__self" && attributeMap.hasOwnProperty(k)) {
                 vals.push(v);
              }
           });
        }

        this.set('context.buffs', vals);
    }.observes('context.stonehearth:buffs'),

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      if (this.get('uri')) {
        this.show();
      } else {
        this.hide();
      }
      
      $('[title]').tipsy();
      
      /*
      $('#commandButtons')
         .off('mouseover', '.commandButton')
         .on('mouseover', '.commandButton', function(event){
         var target = event.target, $cmdBtn;
         if (target.tagName.toLowerCase() == 'img') {
            $cmdBtn = $(target).parent('.commandButton');
         } else {
            $cmdBtn = $(target);
         }
         $cmdBtn.tooltip({
            animation: true
         });
         $cmdBtn.tooltip('show');
      });
      */
   },

   show: function() {
      $('#unitFrame').show();
   },

   hide: function() {
      $('#unitFrame').hide();
   }

});

App.StonehearthCommandButtonView = App.View.extend({
   oneBasedIndex: function() {
      return this.contentIndex + 1;
   }.property(),

   actions: {
      doCommand: function(command) {
         if (!command.enabled) {
            return;
         }
         var target_name = this.get('parentView.context.uri').replace(':','_');
         var event_name = '';

         if (command.action == 'fire_event') {
            // xxx: error checking would be nice!!
            var e = {
               entity : this.get("parentView.uri"),
               event_data : command.event_data
            };
            $(top).trigger(command.event_name, e);
            
            event_name = command.event_name.toString().replace(':','_')

         } else if (command.action == 'call') {
            if (command.object) {
               radiant.call_objv(command.object, command['function'], command.args)            
            } else {
               radiant.callv(command['function'], command.args)
            }
            
            event_name = command['function'].toString().replace(':','_')
            
         } else {
            throw "unknown command.action " + command.action
         }

         //Send analytics
         radiant.call('radiant:send_design_event', 
                      'ui:' + event_name + ':' + target_name );
      }
   }
});