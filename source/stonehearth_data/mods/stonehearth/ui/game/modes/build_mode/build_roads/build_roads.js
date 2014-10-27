App.StonehearthBuildRoadsView = App.View.extend({
   templateName: 'stonehearthBuildRoads',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', "gui"],

   didInsertElement: function() {
      this._super();
      var self = this;

      // tab buttons and pages
      this.$('.toolButton').click(function() {
         var tool = $(this);
         var tabId = tool.attr('tab');
         var tab = self.$('#' + tabId);

         if (!tabId) {
            return;
         }

         // show the correct tab page
         self.$('.tabPage').hide();
         tab.show();

         // activate the tool
         self.$('.toolButton').removeClass('active');
         tool.addClass('active');
         
         // update the material in the tab to reflect the selection
         //self._selectActiveMaterial(tab);
      });

      this.$('#showOverview').click(function() {
         self.showOverview();
      });

      this.$('#showEditor').click(function() {
         self.showEditor();
      });      
   },

   showOverview: function() {
      this.$('#editor').hide();
      this.$('#overview').show();
   },

   showEditor: function() {
      this.$('#editor').show();
      this.$('#overview').hide();
   },

});

