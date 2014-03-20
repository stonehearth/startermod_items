App.StonehearthUnitFrameView = App.View.extend({
	templateName: 'unitFrame',

   components: {
      "stonehearth:commands": {
         commands: []
      },
      "stonehearth:buffs" : {
         "buffs" : {
            "*" : {}
         }
      },
      "stonehearth:inventory" : {},
      "unit_info": {}
   },

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
         if (!self._supressSelection) {
           var selected = data.selected_entity;
           self._selectedEntity = selected;
           
           if (App.getGameMode() == 'normal' && self._selectedEntity) {
              self.set('uri', self._selectedEntity);
              self.show();
           } else {
              //self.set('uri', null);
              self.hide();
           }
         }
      });

      $(top).on('mode_changed.unit_frame', function(_, mode) {
         if (mode == 'normal' && self._selectedEntity) {
            self.show();
         } else {
            self.hide();
         }
      });
   },

   actions: {
      showCharacterSheet: function() {
         $(top).trigger('show_character_sheet.stonehearth', {
            entity: this.get('uri') 
         });
      }
   },

   supressSelection: function(supress) {
      this._supressSelection = supress;
   },

   buffs: function() {
        var vals = [];
        var attributeMap = this.get('context.stonehearth:buffs.buffs');
        
        if (attributeMap) {
           $.each(attributeMap, function(k ,v) {
              if(k != "__self" && attributeMap.hasOwnProperty(k)) {
                 vals.push(v);
              }
           });
        }

        this.set('context.buffs', vals);
    }.observes('context.stonehearth:buffs'),

   inventoryFull: function() {
      var inventory = this.get('context.stonehearth:inventory')
      var full = inventory && inventory.items && inventory.items.length == inventory.capacity
      this.set('context.inventoryFull', full)

   }.observes('context.stonehearth:inventory.items'),

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      if (this.get('uri')) {
        this.show();
      } else {
        this.hide();
      }
      
      this.$('#unitFrame > #buffs').find('.item').each(function() {
        $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
      });

      this.$('#unitFrame > #commandButtons').find('[title]').each(function() {
        $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>' + 
                       '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + $(this).attr('hotkey') + '</span></div>')
         });
      });

      this.$('#inventory').tooltipster({
            content: $('<div class=title>' + i18n.t('stonehearth:inventory_title') + '</div>' + 
                       '<div class=description>' + i18n.t('stonehearth:inventory_description') + '</div>')
        });

      
      this._updateCommandButtons();      
      this._updateInventory();      

      /*
      $('[title]').tooltipster({
        content: $('<span>hi - ' + $(this).attr(title) + '</span>')
      });
  */

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

   _updateCommandButtons: function() {
      var commands = this.get('context.stonehearth:commands.commands');
      if (!commands || commands.length == 0 ) {
        this.$('#commandButtons').hide();
      } else {
        this.$('#commandButtons').show();
      }
   }.observes('context.stonehearth:commands.commands'),

   _updateInventory: function() {
      var inventory = this.get('context.stonehearth:inventory');
      if (!inventory) {
        this.$('#inventory').hide()
      } else {
        this.$('#inventory').show()
      }

   }.observes('context.stonehearth:inventory'),

   show: function() {
      try {

         this.$("#unitFrame").show();
         /*
            .position({
               my : 'center',
               at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + App.stonehearthClient.mouseY,
               of : $(document),
               collision : 'fit'
            });
          */

      } catch (err) {
        console.log(err);
      }
   },

   hide: function() {
      this.$().hide();
   }

});

App.StonehearthCommandButtonView = App.View.extend({
   classNames: ['inlineBlock'],

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