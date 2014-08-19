 $(document).ready(function(){      
      $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
        var unitFrame = App.gameView.getView(App.StonehearthUnitFrameView);
        if (unitFrame) {
          unitFrame.set('uri', data.selected_entity);  
        }
      });
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
      "stonehearth:backpack" : {},
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

   backpackFull: function() {
      var backpack = this.get('context.stonehearth:backpack')
      var full = backpack && backpack.num_items == backpack.capacity
      this.set('context.backpackFull', full)

   }.observes('context.stonehearth:backpack.num_items'),

   didInsertElement: function() {
      var self = this;

      this.$('#unitFrame #buffs').find('.item').each(function() {
        $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
      });

      this.$('#unitFrame #commandButtons').find('[title]').each(function() {
        $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
      });

      this.$('#backpack').tooltipster({
            content: $('<div class=title>' + i18n.t('stonehearth:backpack_title') + '</div>' + 
                       '<div class=description>' + i18n.t('stonehearth:backpack_description') + '</div>')
        });

      
      this.$('.name').click(function(e) {
          radiant.call('stonehearth:camera_look_at_entity', self.get('uri'))
        });

      this._updateCommandButtons();      
      this._updateBackpack();

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

   _updateBackpack: function() {
      if (this.$()) {
        var backpack = this.get('context.stonehearth:backpack');
        if (!backpack) {
          this.$('#backpack').hide()
        } else {
          this.$('#backpack').show()
        }
      }

   }.observes('context.stonehearth:backpack'),

});

App.StonehearthCommandButtonView = App.View.extend({
   classNames: ['inlineBlock'],

   hotkey: function() {
     return "shift+" + (this.contentIndex + 1);
   }.property(),

   oneBasedIndex: function() {
      return this.contentIndex + 1;
   }.property(),

   actions: {
      doCommand: function(command) {
         App.stonehearthClient.doCommand(this.get("parentView.uri"), command);
      }
   }
});