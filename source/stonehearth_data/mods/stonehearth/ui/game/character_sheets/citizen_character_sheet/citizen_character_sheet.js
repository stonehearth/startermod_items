App.StonehearthCitizenCharacterSheetView = App.View.extend({
	templateName: 'citizenCharacterSheet',
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
               'unit_info' : {}   
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
      }
   },

   init: function() {
      this._super();
      this.set('equipment', {});
   },

   all_job_data: null, 
   first_init: true, 
   curr_class: "", 
   curr_level: null, 

   //Every time the job updates, check if it's either the first time
   //or a signficiant change (new class, new job).
   //If any of these cases are true, rebuild the class structure 
   //(Arr, would rather reuse the existing model, but Ember!)
   _updateJobData : function() {
      this.set('currJobIcon', this.get('context.stonehearth:job.class_icon'));
      var new_level = this.get('context.stonehearth:attributes.attributes.total_level.effective_value');
      var new_class = this.get('context.stonehearth:job.job_uri')
      if (this.first_init || 
          this.curr_level != new_level || 
          this.curr_class != new_class) {
         
         this._build_job_data();
         this.set('first_init', false);
         this.set('curr_level', new_level);
         this.set('curr_class', new_class);
      }
      //this._sanitize_job_level_data();
      this._updateAttributes();
   }.observes('context.stonehearth:job'),

   _build_job_data : function () {
      var self = this;
      var job_data_array = [];
      
      //copy the data into a javascript variable
      var job_controller_map = self.get('context.stonehearth:job.job_controllers');
      $.each(job_controller_map, function(k ,v) {
         if(k != "__self" && job_controller_map.hasOwnProperty(k)) {
            var job_data = {
               job_name : v.job_name, 
               is_max_lv : v.is_max_lv, 
               last_gained_lv : v.last_gained_lv,
               level_data : []
            };

            //Go through the levels and put them into an array
            $.each(v.level_data, function(kl, level_value) {
               if( kl != "__self" && v.level_data.hasOwnProperty(kl)) {
                  var each_lv = {
                     level:level_value.level, 
                     perks : []
                  };

                  //Go through the perks and copy them over
                  for (var i=0; i<level_value.perks.length; i++) {
                     var perk = {
                        name : level_value.perks[i].perk_name, 
                        icon : level_value.perks[i].icon, 
                        description: level_value.perks[i].description,
                        unlocked: level_value.level <= v.last_gained_lv 
                     }
                     each_lv.perks.push(perk);
                  }
                  job_data.level_data.push(each_lv);                  
               }
            });
            job_data_array.push(job_data);
         }
      });
      this.set('all_job_data', job_data_array)
   },

   _setFirstJournalEntry: function() {
      var log = this.get('context.stonehearth:personality.log');

      if (log && log.length > 0) {
         this.set('context.firstJournalEntry', log[0]);

         var targetLog = log[0].entries[log[0].entries.length-1]
         this.set('context.firstJournalEntryText', targetLog.text);
         this.set('context.firstJournalEntryTitle', targetLog.title);

         if (targetLog.scoreData != null) {
            this.set('context.firstJournalEntryScore', true);
            this.set('context.firstJournalEntryScoreIsUp', targetLog.scoreData.score_mod > 0);
         } else {
            this.set('context.firstJournalEntryScore', false);
         }
      } else {
         this.set('context.firstJournalEntry', { title: "no entries" });
         this.set('context.firstJournalEntryTitle', 'no entries');
      }

   }.observes('context.stonehearth:personality'),

   _buildBuffsArray: function() {
        var vals = [];
        var map = this.get('context.stonehearth:buffs.buffs');
        
        if (map) {
           $.each(map, function(k ,v) {
              if(k != "__self" && map.hasOwnProperty(k)) {
                 vals.push(v);
              }
           });
        }

        this.set('context.buffs', vals);
    }.observes('context.stonehearth:buffs'),

   _grabEquipment: function() {
      var self = this;
      var slots = ['torso', 'mainhand', 'offhand'];
      var equipment = self.get('context.stonehearth:equipment.equipped_items');
      $.each(slots, function(i, slot) {
         var equipmentPiece = equipment[slot];
         /*
         var slotDiv = self.$('#' + slot + 'Slot');

         if (slotDiv.length == 0) {
            return;
         }

         if (equipmentPiece) {
            slotDiv.html(equipmentPiece.unit_info.name);
         } else {
            slotDiv.html('');
         }*/

         self.set('equipment.' + slot, equipmentPiece);
      });

    }.observes('context.stonehearth:equipment.equipped_items'),

   _setAttributeData: function() {
      //this._updateAttributes();
      Ember.run.scheduleOnce('afterRender', this, '_updateAttributes');

      var new_level = this.get('context.stonehearth:attributes.attributes.total_level.effective_value');
      if (this.curr_level != null && this.curr_level != new_level) {         
         this._build_job_data();
         this.set('curr_level', new_level);
      }

   }.observes('context.stonehearth:attributes, context.stonehearth:buffs'),

   _updateAttributes: function() {
      var self = this;
      var buffsByAttribute = this._sortBuffsByAttribute();

      if (!self.$()) {
         //Ember.run.scheduleOnce('afterRender', this, '_updateAttributes');
         return;
      }

      self.$('.value').each(function(index){
         self._showBuffEffects(this, buffsByAttribute);
      });

      self.$('#glass > div').each(function() {
         self._showBuffEffects(this, buffsByAttribute);
      }); 

      var healthPercent = Math.floor(self.get('context.stonehearth:attributes.attributes.health.effective_value') * 100 / self.get('context.stonehearth:attributes.attributes.max_health.effective_value'))
      var moralePercent = Math.floor(self.get('context.stonehearth:score.scores.happiness.score'));
      var expPercent = Math.floor(self.get('context.stonehearth:job.current_level_exp') * 100 / self.get('context.stonehearth:job.xp_to_next_lv'))
      //self.$('.healthBar').width(healthPercent + '%');
      self.$('.healthBar').width(healthPercent + '%');
      self.$('.moraleBar').width(moralePercent + '%');
      self.$('.expBar').width(expPercent + '%');
   },

   //Call on a jquery object (usually a div) whose ID matches the name of the attribute
   _showBuffEffects: function(obj, buffsByAttribute) {
      var attrib_name = $(obj).attr('id');
      var attrib_value = this.get('context.stonehearth:attributes.attributes.' + attrib_name + '.value');
      var attrib_e_value = this.get('context.stonehearth:attributes.attributes.' + attrib_name + '.effective_value');

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

      //For each buff and debuff that's associated with this attribute, 
      //put it in the tooltip
      if (buffsByAttribute[attrib_name] != null) {
         var tooltipString = '<div class="buffTooltip">';
         for (var i = 0; i < buffsByAttribute[attrib_name].length; i++) {
            tooltipString = tooltipString + '<span class="buffTooltipText">'
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
         tooltipString = tooltipString + '</div>';

         $(obj).tooltipster({
            position: 'right'
         });
         $(obj).tooltipster('content', $(tooltipString));
         $(obj).tooltipster('enable');

      } else if ($(obj).tooltipster()) {
         $(obj).tooltipster('content', "");
         $(obj).tooltipster('disable');
      }
   },

   _sortBuffsByAttribute: function() {
      var allBuffs = this.get('context.stonehearth:buffs.buffs');
      var buffsByAttribute = {};
      if (allBuffs) {
         $.each(allBuffs, function(k ,v) {
            if(k != "__self" && allBuffs.hasOwnProperty(k)) {
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

   didInsertElement: function() {
      var self = this;

      var p = this.get('context.stonehearth:personality');
      var b = this.get('context.stonehearth:buffs');


      this.$('.tab').click(function() {
         var tabPage = $(this).attr('tabPage');

         self.$('.tabPage').hide();
         self.$('.tab').removeClass('active');
         $(this).addClass('active');

         self.$('#' + tabPage).show();
      });

      this.$('#name').focus(function (e) {
         radiant.call('stonehearth:enable_camera_movement', false)
      });

      this.$('#name').blur(function (e) {
         radiant.call('stonehearth:enable_camera_movement', true)
      });

      this.$('#name').keypress(function (e) {
            if (e.which == 13) {
               radiant.call('stonehearth:set_display_name', self.uri, $(this).val())
               $(this).blur();
           }
         });

      this.$('.slot img').tooltipster();
      
      if (p) {
         $('#personality').html($.t(p.personality));   
      }
      if (b) {
         self._updateAttributes();
      }
   },

});