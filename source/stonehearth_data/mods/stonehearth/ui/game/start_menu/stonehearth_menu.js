$.widget( "stonehearth.stonehearthMenu", {
   
   _dataToMenuItemMap: {},
   _foundjobs: {},

   options: {
      // callbacks
      data: {},

      click: function(item) {
         // console.log('Clicked item: ' + item);
      },

      hide: function() {
         // console.log('menu hidden');
      }
   },

   hideMenu: function(menuItem) {
      //this.menu.find('.menuItemGroup').hide();
      this.showMenu(null);
      if (this.options.hide) {
         this.options.hide();
      }
   },

   getMenu: function() {
      return this._currentOpenMenu;
   },

   showMenu: function(id) {
      this.menu.find('.menuItemGroup').hide();
      this.menu.find('.menuItemGroup').width(this.menu.find('.rootGroup').width());      
      
      var nodeData;

      if (id) {
         var subMenu = '[parent="' + id +'"]';
         this.menu.find(subMenu).show();
         nodeData = this._dataToMenuItemMap[id]
      }

      this._currentOpenMenu = id;

      $(top).trigger("start_menu_activated", {
         id: id, 
         nodeData: nodeData
      });
   },

   /*
   setGameMode: function(mode) {
      if (mode == "normal" && this.getGameMode() != "normal") {
         this.hideMenu();
         return;
      }

      if (this.getGameMode() == mode ) {
         return;
      }

      var self = this;
      $.each(this._dataToMenuItemMap, function(key, node) {
         if (node.game_mode == mode) {

            if (self._currentOpenMenu != key) {
               self.showMenu(key);
            }
         }
      });
   },
   */

   getGameMode: function() {
      if (this._currentOpenMenu) {
         return this._dataToMenuItemMap[this._currentOpenMenu].game_mode
      } else {
         return null;
      }
   },

   _create: function() {
      var self = this;

      this._dataToMenuItemMap = {};
      this.menu = $('<div>').addClass('stonehearthMenu');

      this._addItems(this.options.data)

      // a bit of a hack. remove the root group then append it, so it's at the bottom of the menu div
      //var rootGroup = this.menu.find('.rootGroup');
      //this.menu.detach('.rootGroup');
      //this.menu.append(rootGroup);

      this.element.append(this.menu);
      
      this.hideMenu();

      this.menu.on( 'click', '.menuItem', function() {

         // close all open tooltips
         self.menu.find('.menuItem').tooltipster('hide');

         var menuItem = $(this);
         var id = menuItem.attr('id');
         var nodeData = self._dataToMenuItemMap[id]

         if (nodeData.clickSound) {
            radiant.call('radiant:play_sound', {'track' : nodeData.clickSound});
         }

         // deactivate any tools that are open
         App.stonehearthClient.deactivateAllTools();

         // if this menu has sub-items, hide any menus that are open now so we can show a new one
         if (nodeData.items) {
            if (self.getMenu() == id) {
               radiant.call('radiant:play_sound', {'track' : nodeData.menuHideSound} );
               self.hideMenu();
            } else {
               radiant.call('radiant:play_sound', {'track' : nodeData.menuShowSound} );
               self.showMenu(id);
            }
         } else {
            if (!nodeData.sticky) {
               self.hideMenu();
            }            
         }

         // show the parent menu for this menu item
         var parent = menuItem.parent();
         var grandParentId = parent.attr('parent');
         if (grandParentId) {
            self.showMenu(grandParentId);   
         }
         
         if (self.options.click) {
            self.options.click(id, nodeData);
         }
         return false;
      });

      this.menu.on( 'click', '.close', function() {
         self.showMenu(null);
      });

      //this.menu.find('.menuItemGroup').width(this.menu.find('.rootGroup').width());

      /*
      $(document).click(function() {
         if (!App.stonehearthClient.getActiveTool()) {
            self.hideMenus();
         }
      });
      */
   },

   _addItems: function(nodes, parentId, name, depth) {
      if (!nodes) {
         return;
      }

      if (!depth) {
         depth = 0;
      }

      var self = this;
      var groupClass = depth == 0 ? 'rootGroup' : 'menuItemGroup'
      var el = $('<div>').attr('parent', parentId)
                         .addClass(groupClass)
                         .addClass('depth' + depth)
                         .append('<div class=close></div>')
                         .appendTo(self.menu);

      // add a special background div for the root group
      if (depth == 0) {
         el.append('<div class=background></div>');
      }
      
      $.each(nodes, function(key, node) {
         self._dataToMenuItemMap[key] = node;

         var hotkey = node.hotkey || '';
         var description = node.description;
         var name = node.name;

         var item = $('<img>')
                     .attr('id', key)
                     .attr('hotkey', hotkey)
                     .attr('src', node.icon)
                     //.css({ 'background-image' : 'url(' + node.icon + ')' })
                     .addClass('menuItem')
                     .addClass('button')
                     .addClass(node.class)
                     .append('<div class=label>' + node.name + '</div>')
                     .append('<div class=hotkey>' + hotkey + '</div>')
                     .appendTo(el);

         self._buildTooltip(item);

         if (node.items) {

            self._addItems(node.items, key, node.name, depth + 1);
         }

      });

      if (name) {
         $('<div>').html(name)
                   .addClass('header')
                   .appendTo(el);         
      }
   },

   _buildTooltip: function(item) {
      var self = this;
      var id = item.attr('id');
      var node = self._dataToMenuItemMap[id];

      var name = node.name;
      var description = node.description;
      var hotkey = node.hotkey;

      item.tooltipster({
         content: $('<div class=title>' + name + '</div>' + 
                    '<div class=description>' + description + '</div>' + 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + hotkey + '</span></div>')
      });
   }

});
