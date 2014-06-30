$(document).on('stonehearthReady', function(){
   App.debugDock.addToDock(App.StonehearthEntityInspectorIcon);
});

App.StonehearthEntityInspectorIcon = App.View.extend({
   templateName: 'entityInspectorIcon',
   classNames: ['debugDockIcon'],

   didInsertElement: function() {
      this.$().click(function() {
         App.debugView.addView(App.StonehearthEntityInspectorView);
      })
   }
});

App.StonehearthEntityInspectorView = App.View.extend({
   templateName: 'entityInspector',

   init: function() {
      var self = this;
      this._super();

      $(top).on("radiant_selection_changed.entity_inspector", function (_, data) {
         var uri = data.selected_entity;
         if (uri) {
            self.fetch(uri);
         } else {
            self.set('context.state', 'no entity selected');
         }
      });
   },

   destroy: function() {
      var self = this;

      self._destoyed = true;
      if (self.trace) {
         self.trace.destroy();
         self.trace = null;
      }

      //$(top).off("radiant_selection_changed.entity_inspector")
      this._super(); 
   },

   _updateToolTip: function(id, row) {
      var self = this;
      self._inspect_frame_id = id;
      var frame = self._frames[self._inspect_frame_id];
      if (frame) {
         var offset = row.offset();
         self.set('context.ai.inspect_frame', frame);
         this.$().find("#entityInspectorFrameInfo").offset({
            top: offset.top,
            left: offset.left + row.width() + 50,
         })
      } else {
         self.set('context.ai.inspect_frame', null);
      }
   },

   didInsertElement: function() {
      var self = this;

      this.$().on('click', '.row', function() {
         /*
         self._updateToolTip($(this), '<div class=title>' + 'title' + '</div>' + 
                        '<div class=description>' + 'description' + '</div>');
         */
         var id = $(this).attr('row_id');
         var cls  = $(this).attr('class');
         if (id) {
            self._updateToolTip(id, $(this));
         }
      })

      this.$().draggable();

      this.$('.close').click(function() {
         self.destroy();
      });

   },

   fetch: function(entity) {
      var self = this;
      this.set('context', {});
      self.set('context.state', 'fetching ai component uri');
      self.set('context.ai', Ember.Object.create());
      
      if (self.trace) {
         self.trace.destroy();
         self.trace = null;
      }
      self.trace = radiant.trace(entity);
      self.trace
         .progress(function(json) {
            if (self.trace) {
              self.trace.destroy();
              self.trace = null;
            }
            self.set('context.state', 'updating');
            self._aiComponent = json['stonehearth:ai'];
            self.fetchAiComponent();
         })
         .fail(function(e) {
            console.log(e);
         });
   },

   _processDebugInfo: function(node, indent) {
      indent = indent ? indent : 0;
      var self = this;

      if (node instanceof Array || node instanceof Object) {
         node.styleOverride = 'padding-left: ' + (indent * 14) + 'px';

         if (node.activity) {
            self._addStateMetadata(node.execution_units);
         }

         if (node.action) {
            indent++;
            node.parsed_state = self._parseState(node);
            if (node.parsed_state == 'stopped-not_selected' || node.parsed_state == 'stopped-waiting') {
               node.action.execution_frames = null;
            }
         }

         $.each(node, function(k, v) {
            self._processDebugInfo(v, indent);
         });

         if (node instanceof Object) {
            if (node.id) {
              self._frames[node.id] = node;
            }
         }
      }
   },

   _parseState: function(node) {
      if (node.state == 'stopped') {
         if (node.state_metadata == 'not_selected') {
            return 'stopped-not_selected';
         } else {
            return 'stopped-waiting';
         }
      } else {
         return node.state;
      }
   },

   _addStateMetadata: function(executionUnits) {
      var hasRunningUnit = false;

      // are any of the actions running?
      $.each(executionUnits, function(k, executionUnit) {
         if (executionUnit instanceof Object) {
            if (executionUnit.state == 'running') {
               hasRunningUnit = true;
               return false;
            }
         }
         return true;
      });

      // if so, indicate that the other stopped actions lost the election
      if (hasRunningUnit) {
         $.each(executionUnits, function(k, executionUnit) {
            if (executionUnit instanceof Object) {
               if (executionUnit.state == 'stopped') {
                  executionUnit.state_metadata = 'not_selected';
               }
            }
         });
      }
   },

   fetchAiComponent: function() {
      if (this._destoyed) {
         return;
      }

      var self = this;
      self.set('context.state', 'updating...');
      radiant.call_obj(self._aiComponent, 'get_debug_info')
        .done(function(debugInfo) {
            self.set('context.state', 'updating..');
            self._frames = {};
            self._processDebugInfo(debugInfo);
            if (self._frames[self._inspect_frame_id]) {
               debugInfo.inspect_frame = self._frames[self._inspect_frame_id];
            }
            self.set('context.ai', debugInfo);
            if (self._aiComponent) {
               self.set('context.state', 'updating. ' + new Date().getTime());
               self._fetchTimer = setTimeout(function() {
                 self.fetchAiComponent();
               }, 500);
               self.set('context.state', 'updating');
            } else {
               self.set('context.state', 'idle.');
            }
         });
   },

   actions: {
      close: function() {
        this.destroy();
      }
   }
});
