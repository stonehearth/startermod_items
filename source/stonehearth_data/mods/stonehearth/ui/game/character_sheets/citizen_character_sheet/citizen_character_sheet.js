App.StonehearthCitizenCharacterSheetView = App.View.extend({
	templateName: 'citizenCharacterSheet',
   uriProperty: 'model',
   closeOnEsc: true,

   components: {
      'unit_info': {},
      'stonehearth:buffs' : {
         'buffs' : {
            '*' : {}
         }
      },
      
      'stonehearth:equipment' : {
         'equipped_items' : {
            '*' : {
               'uri': {},
               'unit_info' : {},
            }
         }
      },
      
      'stonehearth:attributes' : {},
      'stonehearth:personality' : {},
      'stonehearth:score' : {}, 
      
      
      'stonehearth:job' : {
         'curr_job_controller' : {},
         'job_controllers' : {
            '*' : {}
         }
      },

      'stonehearth:storage' : {
         'item_tracker' : {}
      }
   },

   init: function() {
      this._super();
      this.set('equipment', {});
      this._createJobDataArray();
   },

   //When we get job data from the server, turn maps into arrays and store in allJobData
   _createJobDataArray: function() {
      var self = this;

      var components = {
         "jobs" : {
            "*" : {
               "description" : {} 
            }
         }
      };

      self._jobPerkTrace = new StonehearthDataTrace('stonehearth:jobs:index', components);
      self._jobPerkTrace.progress(function(eobj){
         //Make the table of data
         radiant.each(eobj.jobs, function (key, job) {
            if (job.description.level_data) {
               var levelArray = radiant.map_to_array(job.description.level_data);
               job.description.levelArray = levelArray;
            }
         })

         var jobArray = radiant.map_to_array(eobj.jobs);
         self.set('allJobData', jobArray);
         self._jobPerkTrace.destroy()
      });
   },

   //Updates job related attributes
   _updateJobData : function() {
      this.set('currJobIcon', this.get('model.stonehearth:job.class_icon'));
      Ember.run.scheduleOnce('afterRender', this, '_updateAttributes');
   }.observes('model.stonehearth:job'),

   //Updates perk table
   _updateJobDataDetails: function() {
      Ember.run.scheduleOnce('afterRender', this, '_updateJobsAndPerks');
   }.observes('model.stonehearth:job.curr_job_controller'),

   //Go through each job we've had and annotate the perk table accordingly
   _updateJobsAndPerks : function() {
      var self = this;

      //Hide all the job divs before selectively showing the ones for the current
      //character.
      self.$('.jobData').hide();
      
      //show each class that this person has ever been
      var jobs = this.get('model.stonehearth:job.job_controllers');
      radiant.each(jobs, function(alias, data) {
         if (alias != "stonehearth:jobs:worker") {
            var div = self.$("[uri='" + alias + "']");
            
            //For each, figure out which perks should be unlocked
            self._unlockPerksToLevel(div, data.last_gained_lv)

            $(div).show();
         }
      });

      //Highlight current class, since it needs to be 100% up to date
      self.$('.activeClassNameHeader').removeClass('activeClassNameHeader');
      self.$('.className').addClass('retiredClassNameHeader');
      self.$('.jobData').addClass('retiredEffect');
      var currClassAlias = this.get('model.stonehearth:job.job_uri');
      var $currClass = self.$("[uri='" + currClassAlias + "']");
      $currClass.prependTo("#citizenCharacterSheet #abilitiesTab");
      $currClass.find('.className').removeClass('retiredClassNameHeader').addClass('activeClassNameHeader');
      $currClass.removeClass('retiredEffect');
      //$currClass.removeClass('retiredClassNameHeader').addClass('activeClassNameHeader');
      self._unlockPerksToLevel($currClass,  this.get('model.stonehearth:job.curr_job_controller.last_gained_lv'))
      $currClass.find('.retiredAt').hide();

      //Make the job tooltips
      this._updateJobTooltips();
   },

   //Given a perk div and target level, change the classes within to reflect the current level
   _unlockPerksToLevel : function (target_div, target_level) {
      $(target_div).find('.levelLabel').addClass('lvLabelLocked');
      $(target_div).find('img').addClass('perkImgLocked');
      for(var i=0; i<=target_level; i++) {
         $(target_div).find("[imgLevel='" + i + "']").removeClass('perkImgLocked').addClass('perkImgUnlocked');
         $(target_div).find("[lbLevel='" + i + "']").removeClass('lvLabelLocked').addClass('lvLabelUnlocked');
         $(target_div).find("[divLevel='" + i + "']").attr('locked', "false");
      }

      //For good measure, throw the level into the class name header or remove if the level is 0
      if (target_level == 0) {
         $(target_div).find('.lvlTitle').text(i18n.t('stonehearth:apprentice'));
      } else {
         $(target_div).find('.lvlTitle').text(i18n.t('stonehearth:level_text') + target_level );
      }

      //Calculate the height of the jobPerks section based on the number of perkDivs
      //TODO: factor these magic numbers out or change if/when the icons change size
      var numPerks = $(target_div).find('.perkDiv').length;
      if (numPerks == 0) {
         $(target_div).find('.jobPerks').css('height', '0px');
      } else {
         var num_rows = parseInt(numPerks/8) + 1;
         var total_height = num_rows * 90;
         $(target_div).find('.jobPerks').css('height', total_height + 'px');
      }

      $(target_div).find('.retiredAt').show();
   },

   //Make tooltips for the perks
   _updateJobTooltips : function() {
      var self = this;
      $('.tooltip').tooltipster();
      $('.perkDiv').each(function(index){
         var perkName = $(this).attr('name');
         var perkDescription = $(this).attr('description');
         var tooltipString = '<div class="perkTooltip"> <h2>' + perkName;

         //If we're locked then add the locked label
         if ($(this).attr('locked') == "true") {
            tooltipString = tooltipString + '<span class="lockedTooltipLabel">' + i18n.t('stonehearth:locked_status') + '</span>';
         }

         tooltipString = tooltipString + '</h2>'+ perkDescription + '</div>';
         $(this).tooltipster('content', $(tooltipString));
      });
 
   },

   _setFirstJournalEntry: function() {
      var log = this.get('model.stonehearth:personality.log');

      if (log && log.length > 0) {
         this.set('model.firstJournalEntry', log[0]);

         var targetLog = log[0].entries[log[0].entries.length-1]
         this.set('model.firstJournalEntryText', targetLog.text);
         this.set('model.firstJournalEntryTitle', targetLog.title);

         if (targetLog.scoreData != null) {
            this.set('model.firstJournalEntryScore', true);
            this.set('model.firstJournalEntryScoreIsUp', targetLog.scoreData.score_mod > 0);
         } else {
            this.set('model.firstJournalEntryScore', false);
         }
      } else {
         this.set('model.firstJournalEntry', { title: "no entries" });
         this.set('model.firstJournalEntryTitle', 'no entries');
      }

   }.observes('model.stonehearth:personality'),

   _buildBuffsArray: function() {
        var vals = [];
        var map = this.get('model.stonehearth:buffs.buffs');
        vals = radiant.map_to_array(map);
        this.set('model.buffs', vals);
    }.observes('model.stonehearth:buffs'),

   _setEquipmentData: function() {
      var self = this;
      var slots = ['torso', 'mainhand', 'offhand'];
      var equipment = self.get('model.stonehearth:equipment.equipped_items');
      var allEquipment = [];
      radiant.each(slots, function(i, slot) {
         var equipmentPiece = equipment[slot];

         if (equipmentPiece) {
            var equipmentInfo = {
               equipment: equipmentPiece,
               slotId: slot + "Slot",
            }
            allEquipment.push(equipmentInfo);
         }
         
      });
      self.set('all_equipment', allEquipment);
    }.observes('model.stonehearth:equipment.equipped_items'),

    _equipmentUpdatedListener: function() {
      var self = this;
      Ember.run.scheduleOnce('afterRender', this, function() {
         self._updateEquipmentTooltips();
       });
    }.observes('all_equipment'),

   _updateEquipmentTooltips: function() {
      var self = this;
      var all_equipment = self.get('all_equipment');
      radiant.each(all_equipment, function(i, equipmentInfo) {
         var equipmentRow = self.$('#' + equipmentInfo.slotId);
         var equipmentPiece = equipmentInfo.equipment;
         if (equipmentRow && equipmentRow.length != 0) {
            var tooltipString = '<div class="detailedTooltip"> <h2>' + equipmentPiece.unit_info.name
                                 + '</h2>'+ equipmentPiece.unit_info.description + '</div>';
            equipmentRow.tooltipster({content: $(tooltipString)});
         }
      });
   },

    //When the attribute data changes, update the bars
   _setAttributeData: function() {
      Ember.run.scheduleOnce('afterRender', this, '_updateAttributes');
   }.observes('model.stonehearth:attributes' , 'model.stonehearth:buffs'),

   _updateAttributes: function() {
      var self = this;
      var buffsByAttribute = this._sortBuffsByAttribute();

      if (!self.$()) {
         //Ember.run.scheduleOnce('afterRender', this, '_updateAttributes');
         return;
      }

      self.$('.attr').each(function(index){
         self._showBuffEffects(this, buffsByAttribute, true);
      });

      self.$('.score').each(function(index){
         self._showBuffEffects(this, buffsByAttribute, false);
      });

      self.$('#glass > div').each(function() {
         self._showBuffEffects(this, buffsByAttribute, true);
      }); 

      var healthPercent = Math.floor(self.get('model.stonehearth:attributes.attributes.health.user_visible_value') * 100 / self.get('model.stonehearth:attributes.attributes.max_health.user_visible_value'))
      var moralePercent = Math.floor(self.get('model.stonehearth:score.scores.happiness.score'));
      var expPercent = Math.floor(self.get('model.stonehearth:job.current_level_exp') * 100 / self.get('model.stonehearth:job.xp_to_next_lv'))
      self.$('.healthBar').width(healthPercent + '%');
      self.$('.moraleBar').width(moralePercent + '%');
      self.$('.expBar').width(expPercent + '%');

      self.$('#buffs').find('.item').each(function(){
         $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
      });
   },

   //Call on a jquery object (usually a div) whose ID matches the name of the attribute
   _showBuffEffects: function(obj, buffsByAttribute, isAttribute) {
      var attrib_name = $(obj).attr('id');
      var attrib_value = this.get('model.stonehearth:attributes.attributes.' + attrib_name + '.value');
      var attrib_e_value = this.get('model.stonehearth:attributes.attributes.' + attrib_name + '.user_visible_value');

      if (attrib_e_value > attrib_value) {
         //If buff, make text green
         $(obj).removeClass('debuffedValue normalValue').addClass('buffedValue');
      } else if (attrib_e_value < attrib_value) {
         //If debuff, make text yellow
         $(obj).removeClass('buffedValue normalValue').addClass('debuffedValue');
      } else if (attrib_e_value == attrib_value) {
         //If nothing, keep steady
         $(obj).removeClass('debuffedValue buffedValue').addClass('normalValue');
      }

      var hasTooltip = isAttribute ? App.tooltipHelper.hasAttributeTooltip(attrib_name) :  App.tooltipHelper.hasScoreTooltip(attrib_name);
      if (hasTooltip) {
         //For each buff and debuff that's associated with this attribute, 
         //put it in the tooltip
         if (buffsByAttribute[attrib_name] != null) {
            var buffString = '<div class="buffTooltip">';
            for (var i = 0; i < buffsByAttribute[attrib_name].length; i++) {
               buffString = buffString + '<span class="buffTooltipText">'
                                                   + '<span class="dataSpan ' 
                                                   + buffsByAttribute[attrib_name][i].type
                                                   + 'DataSpan">'
                                                   + buffsByAttribute[attrib_name][i].shortDescription
                                                   + '</span>'
                                                   + '<img class="buffTooltipImg" src="'
                                                   + buffsByAttribute[attrib_name][i].icon + '"> ' 
                                                   + buffsByAttribute[attrib_name][i].name
                                                   + '</span></br>';
            }
            buffString = buffString + '</div>';
         }

         var tooltipString = isAttribute ? 
                  App.tooltipHelper.getAttributeTooltip(attrib_name, buffString) : App.tooltipHelper.getScoreTooltip(attrib_name, false, buffString)
         
         $(obj).tooltipster({
               position: 'right'
            });
         $(obj).tooltipster('content', $(tooltipString));
         $(obj).tooltipster('enable');
      }
   },

   _sortBuffsByAttribute: function() {
      var allBuffs = this.get('model.stonehearth:buffs.buffs');
      var buffsByAttribute = {};
      if (allBuffs) {
         radiant.each(allBuffs, function(k ,v) {
            //If the buff is private don't add it. Public buffs can be undefined or is_private = false
            if (allBuffs[k].invisible_to_player == undefined || !allBuffs[k].invisible_to_player) {
               var modifiers = allBuffs[k].modifiers;
               for (var mod in modifiers) {
                  var new_buff_data = {}
                  new_buff_data.name = allBuffs[k].name;
                  new_buff_data.type = allBuffs[k].type;
                  new_buff_data.icon = allBuffs[k].icon;
                  new_buff_data.shortDescription = "";
                  if (allBuffs[k].short_description != undefined) {
                     new_buff_data.shortDescription = allBuffs[k].short_description;                     
                  } else {
                     for (var attrib in modifiers[mod]) {
                        if (attrib == 'multiply' || attrib == 'divide') {
                           var number = 1 - modifiers[mod][attrib];
                           number = number * 100
                           var rounded = Math.round( number * 10 ) / 10;
                           rounded = Math.abs(rounded);
                           new_buff_data.shortDescription += rounded + '% '; 
                        } else if (attrib == 'add') {
                           var number = modifiers[mod][attrib];
                           new_buff_data.shortDescription += '+' + number + ' '; 
                        } else if (attrib == 'subtract') {
                           var number = modifiers[mod][attrib];
                           new_buff_data.shortDescription += '-' + number + ' '; 
                        }
                     }
                  }
                  //There are so many ways to modify a buff; let writer pick string
                  if (buffsByAttribute[mod] == null) {
                     buffsByAttribute[mod] = [];
                  }
                  buffsByAttribute[mod].push(new_buff_data);
               }
            }
         });
      }
      return buffsByAttribute;
   },

   _updateMorale: function() {
      var self = this;
      var scoresToUpdate = ['happiness', 'food', 'shelter', 'safety'];
      radiant.each(scoresToUpdate, function(i, score_name) {
         var score_value = self.get('model.stonehearth:score.scores.' + score_name + '.score');
         self.set('score_' + score_name, Math.round(score_value) / 10);
      });
   }.observes('model.stonehearth:score'),

   _updateBackpackItems : function() {
      var self = this;
      Ember.run.scheduleOnce('afterRender', this, function() {
         if (!self._backpackItemsPalette) {
            // When moving a crate, this function will fire, but no UI will be present.  For now, be lazy
            // and just ignore this case.
            return;
         }
         var tracker = self.get('model.stonehearth:storage.item_tracker');
         self._backpackItemsPalette.stonehearthItemPalette('updateItems', tracker.tracking_data);
      });
   }.observes('model.stonehearth:storage.item_tracker'),

   didInsertElement: function() {
      var self = this;

      var p = this.get('model.stonehearth:personality');
      var b = this.get('model.stonehearth:buffs');

      // have the character sheet tract the selected entity.
      $(top).on("radiant_selection_changed.citizen_character_sheet", function (_, data) {
         self._onEntitySelected(data);
      });

      new StonehearthInputHelper(this.$('#name'), function (value) {
            radiant.call('stonehearth:set_display_name', self.uri, value)
         });
      
      if (p) {
         $('#personality').html($.t(p.personality));   
      }
      if (b) {
         self._updateAttributes();
      }

      this._backpackItemsPalette = this.$('#backpackItemsPalette').stonehearthItemPalette({
         cssClass: 'inventoryItem',
      });
   },

   _onEntitySelected: function(e) {
      var self = this;
      var entity = e.selected_entity
      
      if (!entity) {
         self.destroy();
      }

      // nuke the old trace
      if (self.selectedEntityTrace) {
         self.selectedEntityTrace.destroy();
      }

      // trace the properties so we can tell if we need to popup the properties window for the object
      self.selectedEntityTrace = radiant.trace(entity)
         .progress(function(result) {
            self._examineEntity(result);
         })
         .fail(function(e) {
            console.log(e);
         });
   },

   _examineEntity: function(entity) {
      var self = this;

      if (!entity) {
         self.destroy();
      }

      if (entity['stonehearth:job']) {
         self.set('uri', entity.__self);
      } else  {
         self.destroy();
      }
   },

   destroy: function() {
      $(top).off("radiant_selection_changed.citizen_character_sheet")

      if (this.selectedEntityTrace) {
         this.selectedEntityTrace.destroy();
      }

      this._super();
   }
});