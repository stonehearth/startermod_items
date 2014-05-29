
$(document).ready(function(){
  var unitFrame = {
      view: null,
      uri: null
  }

  $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
     unitFrame.uri = data.selected_entity;
     refreshUnitFrame();
  });  

  $(top).on('mode_changed', function(_, mode) {
     refreshUnitFrame();
  });

  function refreshUnitFrame() {
     var mode = App.getGameMode();

     // nuke the old unit frame
     if (unitFrame.view) {
        unitFrame.view.destroy();
     }

     
     //if (mode != 'build' && mode != 'zones' && unitFrame.uri) {
      if (unitFrame.uri) {
        unitFrame.view = App.gameView.addView(App.StonehearthUnitFrameView, { uri: unitFrame.uri });
     }     
  }
});

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
   },

   actions: {
      showCharacterSheet: function() {
         $(top).trigger('show_character_sheet.stonehearth', {
            entity: this.get('uri') 
         });
      }
   },

   _updateVisibility: function() {
      var self = this;
      var selectedEntity = this.get('uri');
      if (App.getGameMode() == 'normal' && selectedEntity) {
        this.set('visible', true);
      } else {
        this.set('visible', false);
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
      var self = this;
      this.$('#unitFrame > #buffs').find('.item').each(function() {
        $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
      });

      this.$('#unitFrame > #commandButtons').find('[title]').each(function() {
        $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
      });

      this.$('#inventory').tooltipster({
            content: $('<div class=title>' + i18n.t('stonehearth:inventory_title') + '</div>' + 
                       '<div class=description>' + i18n.t('stonehearth:inventory_description') + '</div>')
        });

      
      this.$('.name').click(function(e) {
          radiant.call('stonehearth:camera_look_at_entity', self.get('uri'))
        });

      this._updateCommandButtons();      
      this._updateInventory();
   },

   _updateCommandButtons: function() {
      if (this.$()) {
        var commands = this.get('context.stonehearth:commands.commands');
        if (!commands || commands.length == 0 ) {
          this.$('#commandButtons').hide();
        } else {
          this.$('#commandButtons').show();
        }
      }
   }.observes('context.stonehearth:commands.commands'),

   _updateInventory: function() {
      if (this.$()) {
        var inventory = this.get('context.stonehearth:inventory');
        if (!inventory) {
          this.$('#inventory').hide()
        } else {
          this.$('#inventory').show()
        }
      }

   }.observes('context.stonehearth:inventory'),

});

App.StonehearthCommandButtonView = App.View.extend({
   classNames: ['inlineBlock'],

   oneBasedIndex: function() {
      return this.contentIndex + 1;
   }.property(),

   actions: {
      doCommand: function(command) {
         App.stonehearthClient.doCommand(this.get("parentView.uri"), command);
      }
   }
});