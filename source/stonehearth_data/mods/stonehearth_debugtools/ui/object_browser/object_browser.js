var topElement;
$(document).on('stonehearthReady', function(){
   topElement = $(top);
   App.debugDock.addToDock(App.StonehearthObjectBrowserIcon);
});

App.StonehearthObjectBrowserIcon = App.View.extend({
   templateName: 'objectBrowserIcon',
   classNames: ['debugDockIcon'],

   didInsertElement: function() {
      this.$().click(function() {
         App.debugView.addView(App.StonehearthObjectBrowserView, { trackSelected: true })
      })
   }

});

App.StonehearthObjectBrowserView = App.View.extend({
   templateName: 'objectBrowser',
   uriProperty: 'model',
   subViewClass: 'stonehearthObjectBrowserRaw',
   history: [],
   forwardHistory: [],

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      // for some reason $(top) here isn't [ Window ] like everywhere else.  Why?  Dunno.
      // So annoying!  Use the cached value of $(top) we got in 'stonehearthReady'
      topElement.on("radiant_selection_changed.object_browser", function (_, data) {
         if (self.trackSelected) {
            var uri = data.selected_entity;
            if (uri) {
               self.fetch(uri);
            }
         }
      });

      this.$().draggable();

      if (self.relativeTo) {
         var top = self.relativeTo.top + 64;
         var left = self.relativeTo.left + 64;
         this.$().css({'top': top, 'left' : left})
      }
      this.$('.button').tooltipster({
         theme: 'tooltipster-shdt',
         arrow: true,
      });

      this.$('#body').on("click", "a", function(event) {
         var uri = $(this).attr('href');

         event.preventDefault();
         if (event.shiftKey) {
            App.debugView.addView(App.StonehearthObjectBrowserView, {
                  uri: uri,
                  relativeTo: self.$().offset(),
               })
         } else {
            self.fetch(uri);
         }
      });

      $('#uriInput').keypress(function(e) {
         if (e.which == 13) {
            $(this).blur();
            self.fetch($(this).val());
         }
      });

      self._updateTrackSelectedControl();
      self._updateCollapsedControl();
   },

   destroy: function() {
      this._super();
   },

   fetch: function(uri) {
      this.history.push(uri);
      this.set('uri', uri);
   },

   raw_view: function() {
      var model = this.get('model');

      if (!model) {
         return '';
      }
      var json = JSON.stringify(model, undefined, 2);
      return json.replace(/&/g, '&amp;')
                 .replace(/</g, '&lt;')
                 .replace(/>/g, '&gt;')
                 .replace(/ /g, '&nbsp;')
                 .replace(/\n/g, '<br>')
                 .replace(/"(object:\/\/[^"]*)"/g, '<a href="$1">$1</a>')
                 //.replace(/(\/[^"]*)/g, '<a href="$1">$1</a>')
   }.property('model'),


   _update_template: function() {
      var type = this.get('model.type');
      var subViewClass = 'stonehearthObjectBrowserRaw';

      if (type == 'stonehearth:encounter') {
         subViewClass = 'stonehearthObjectBrowserEncounter';
      }
      this.set('subViewClass', subViewClass);
      this.rerender();
   }.observes('model'),


   _updateTrackSelectedControl : function() {
      var self = this;
      if (self.trackSelected) {
         var selected = App.stonehearthClient.getSelectedEntity();
         if (selected) {
            self.set('uri', selected)
         }
         this.$("#trackSelected").addClass('depressed');
      } else {
         this.$("#trackSelected").removeClass('depressed');
      }
   },

   _updateCollapsedControl : function() {
      var self = this;
      if (self.collapsed) {
         this.$('#collapse_image').addClass('fa-chevron-up')
                                  .removeClass('fa-chevron-down');
         this.$('#body').hide();
      } else {
         this.$('#collapse_image').removeClass('fa-chevron-up')
                                  .addClass('fa-chevron-down');
         this.$('#body').show();
      }
   },

   actions: {
      goBack: function() {
         if (this.history.length > 1) {
            this.forwardHistory.push(this.history.pop())
         }
         this.fetch(this.history.pop());
      },

      goForward: function () {
         if (this.forwardHistory.length > 0) {
            var uri = this.forwardHistory.pop();
            this.fetch(uri);
         }
      },

      close: function () {
         this.destroy();
      },

      toggleCollapsed: function() {
         var self = this;
         self.collapsed = !self.collapsed;
         self._updateCollapsedControl();
      },

      toggleTrackSelected: function() {
         var self = this;
         self.trackSelected = !self.trackSelected;
         self._updateTrackSelectedControl();
      }
   }
});


App.StonehearthObjectBrowserRawView = App.View.extend({
   templateName: 'objectBrowserRaw',
   uriProperty: 'model',

   raw_view: function() {
      var model = this.get('model');

      if (!model) {
         return '';
      }
      var json = JSON.stringify(model, undefined, 2);
      return json.replace(/&/g, '&amp;')
                 .replace(/</g, '&lt;')
                 .replace(/>/g, '&gt;')
                 .replace(/ /g, '&nbsp;')
                 .replace(/\n/g, '<br>')
                 .replace(/"(object:\/\/[^"]*)"/g, '<a href="$1">$1</a>')
                 //.replace(/(\/[^"]*)/g, '<a href="$1">$1</a>')
   }.property('model'),
});


App.StonehearthObjectBrowserEncounterView = App.View.extend({
   templateName: 'objectBrowserEncounter',
   uriProperty: 'model',
   components: {
      'ctx' : {}
   },
});

