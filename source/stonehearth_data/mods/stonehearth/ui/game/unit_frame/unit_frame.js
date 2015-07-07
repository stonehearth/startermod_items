 $(document).ready(function(){      
   $(top).on("radiant_selection_changed.unit_frame", function (_, data) {
      if (!App.gameView) {
         return;
      }
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
      "stonehearth:job" : {
      },
      "stonehearth:buffs" : {
         "buffs" : {
            "*" : {}
         }
      },
      "unit_info": {},
      "item": {},
      "stonehearth:material": {}
   },

   init: function() {
      this._super();
      var self = this;
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
         radiant.each(attributeMap, function(k ,v) {
            //only push public buffs (buffs who have an is_private unset or false)
            if (v.invisible_to_player == undefined || !v.invisible_to_player) {
               vals.push(v);
            }
         });
      }

      this.set('context.buffs', vals);
   }.observes('context.stonehearth:buffs'),

   didInsertElement: function() {
      var self = this;

      self.$("#nameBlock .name").tooltipster();
      self.$("#portrait").tooltipster();

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

      this.$('.name').click(function() {
        radiant.call('stonehearth:camera_look_at_entity', self.get('uri'))
      });

      this.$('#portrait').click(function (){
         App.stonehearthClient.showCharacterSheet(self.get('uri')); 
      });

      this._updateCommandButtons();
   },

   _updateCommandButtons: function() {
      if (!this.$()) {
         return;
      }
      var commands = this.get('context.stonehearth:commands.commands');
      if (!commands || commands.length == 0 ) {
         this.$('#commandButtons').hide();
      } else {
         this.$('#commandButtons').show();
      }
   }.observes('context.stonehearth:commands.commands'),

   _updateMoneyDescription: function() {
      // Specifically, if the item selected is money, update its description to account for its value.
      var material = this.get('context.stonehearth:material');
      if (material && material.tags_string.indexOf('money') >= 0) {
         var itemComponent = this.get('context.item');
         if (itemComponent) {
            var stacks = itemComponent.stacks;
            this.set("context.unit_info.description", i18n.t('unit_info_gold_description', {stacks: stacks}));
         }
      }
   }.observes('context.item'),

   _updatePortrait: function() {
      if (!this.$()) {
       return;
      }
      var uri = this.uri;
      var job = this.get('context.stonehearth:job');
      if (uri && job) {
         var portrait_url = '/r/get_portrait/?type=headshot&animation=idle_breathe.json&entity=' + uri;
         this.set('portraitSrc', portrait_url);
         this.$('#portrait').show();
      } else {
         this.set('portraitSrc', "");
         this.$('#portrait').hide();
      }
   }.observes('context.stonehearth:job'),
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