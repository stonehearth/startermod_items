(function ($) {

   $.widget("stonehearth.boxmenu", {
      options: {},
      _numButtons: 0,
      _initialMenuBuild: true,
      _clickHandlers: {},

      update: function (items) {
         //this._removeItems();
         //this._updateItems(items);
      },

      _create: function () {
         var self = this;
         var o = this.options;
         //this.element.append("this is the menu<br>");

         // build the container
         this._menu = $('<div class="boxmenu"></div>');

         this.addMenus(o.menu);
         this._initialMenuBuild = false;
         this.element.append(this._menu);

         // event handlers
         $('body').click(function(e) {
            console.log('click!');
            var container = self.element;
            var clickedElement = e.target;

            // super hack alert.
            // for some reason, the new badges aren't regarded as being
            // contained in self.element, so just special case them.
            if ($(clickedElement).hasClass('new')) {
               return;
            }

            // if the target of the click isn't the container...
            // ... nor a descendant of the container
            if (!container.is(clickedElement) && container.has(clickedElement).length === 0) {
               self._hideAllMenus();
            }
         });


         this._menu.delegate('.clickable','mouseenter mouseleave', function(event) {
            if(event.type === 'mouseenter') {
               $(this).find('.hotkey').fadeIn({duration: 150});
            } else {
               $(this).find('.hotkey').fadeOut({duration: 150});
            }

         });


         //this._menu.find('.button').on('click', function() {
         this._menu.delegate('.clickable', 'click', function() {
            var menuItem = $(this);
            var id = menuItem.attr('id');

            var active = true;
            if (menuItem.hasClass('hasMenu')) {
               if (menuItem.hasClass('menuButton') && menuItem.hasClass('active')) {
                  active = false;
                  self._hideAllMenus();
               } else {
                  self._showMenu(id);
               }

            } else {
               // we're a leaf menu item, so close all the menus
               self._hideAllMenus();
               menuItem.addClass('invoked');
            }

            if (active) {
               menuItem.addClass('active');
               menuItem.find('.arrow').show();
            }

            // remove the new badge
            self._removeNewBadge(menuItem);

            // finally, run the function assigned to this menu item
            if (self._clickHandlers[id]) {
               self._clickHandlers[id]();
            }


         });


      },

      addMenus: function(data) {
         var self = this;
         $.each( data, function( k, v ) {
            self._addTopButton(k, v);
         });

      },

      click: function(menuItemId) {
         var menuItem = this._menu.find('#' + menuItemId);
         menuItem.click();
      },

      _addTopButton: function(buttonId, data) {
         var buttonDy = 80;

         var button = this._menu.find("#" + buttonId);

         if (button.length == 0) {
            button = $('<div></div>')
               .addClass('menuButton')
               .addClass('hasMenu')
               .addClass('clickable')
               .attr('id', buttonId)
               .attr('title', data.name)
               .attr('hotkey', data.hotkey)
               .css('bottom', this._numButtons * 80)
               .html('<img src="' + data.icon + '"/>')
               .append('<div class=label>' + data.name + '</div>')
               .append('<div class=hotkey>' + data.hotkey + '</div>');


               /*
               .tooltip({
                  delay: { show: 400, hide: 0 },
                  placement: 'left'
               });
               */

            $('<div class=arrow></div>')
               .hide()
               .appendTo(button)



            if (!this._initialMenuBuild) {
               // this is a new item that has come in after
               // the first build of the menu, so slap a "new"
               // indicator on it
               this._addNewBadge(button);
            }

            this._menu.append(button);

            this._numButtons++;

         } else {
            // new menus are being added to this button, so
            // slap on the "new" indicator
            this._addNewBadge(button);
         }

         this._addSubMenus(buttonId, data);
      },

      _addSubMenus: function(parentId, data) {
         if (!data.items) {
            return;
         }

         // create the menu if necessary
         var selector = '[parent="' + parentId +'"]';
         var menu = this._menu.find(selector);

         if (menu.length == 0) {
            menu = $('<div></div>')
               .addClass('menu')
               .attr('parent', parentId)
               .css({ position: 'absolute'})
               .hide();

            this._menu.append(menu);
         }

         // add the menu items
         var self = this;
         $.each( data.items, function( k, v ) {
            var menuItem = menu.find('#' + k);

            if (menuItem.length == 0) {
               var menuItem = $('<div></div>')
                  .attr('id', k)
                  .attr('hotkey', v.hotkey)
                  .addClass('menuItem')
                  .addClass('clickable')
                  .css('left', menu.find('.menuItem').length * 90)
                  .html('<img class=icon src="' + v.icon + '"/>')
                  .append('<div class=label>' + v.name + '</div>')
                  .append('<div class=hotkey>' + v.hotkey + '</div>');


               // wire up the click handler
               if (v.click) {
                  self._clickHandlers[k] = v.click;
               }

               // style items that spawn a new menu when clicked
               if (v.items) {
                  menuItem.addClass('hasMenu')

                  $('<div class=arrow></div>')
                     .hide()
                     .appendTo(menuItem)
               }

               if (!self._initialMenuBuild) {
                  // this is a new menu item that has come in after
                  // the first build of the menu, so slap a "new"
                  // indicator on it
                  self._addNewBadge(menuItem);
               }

               menu.append(menuItem);
            } else {
               // new menus are being added to this menu item, so
               // slap on the "new" indicator
               self._addNewBadge(menuItem);
            }

            self._addSubMenus(k, v);
         });
      },

      _addNewBadge: function(element) {
         return $('<div></div>')
            .addClass('new')
            .html('new!')
            .appendTo(element);
      },

      _removeNewBadge: function(element) {
         element.find('.new').remove();
      },

      _showMenu: function(buttonId) {
         //console.log('showmenu ' + buttonId)
         var menu = this._menu.find("[parent='" + buttonId + "']" );
         var parentButton = this._menu.find("#" + buttonId);
         var pos = parentButton.position();

         this._hideNonAncestorMenus(buttonId);

         // size the menu correctly
         menu.css({
            width: menu.find('.menuItem').length * 90,
            height: 60
         });

         if (parentButton.hasClass('menuButton')) {
            // position the menu relative to the top button
            var parentButton = this._menu.find("#" + buttonId);
            menu.css({ left: pos.left - (menu.width() + 24), top: pos.top + 8});
         } else {
            // position the menu relative the the menu (the button's parent)
            var parentMenuPos = parentButton.parent().position();
            var parentPos = parentButton.position();

            var rightGuttar = -95;

            var left = Math.min(parentMenuPos.left + parentPos.left,
               rightGuttar - menu.width());

            menu.css({ left: left , top: parentMenuPos.top - 80});
         }

         this._activeMenu = menu;
         menu.show();
      },

      _hideNonAncestorMenus: function(buttonId) {
         var button = $('#' + buttonId);

         var self = this;
         this._menu.find('.menu, .menuButton').each(function(i, m) {
            var menu = $(m);
            var found = self._contains(menu.attr('parent'), buttonId);
            if (!found) {
               if (menu.hasClass('menu')) {
                  menu.hide();
               }

               menu.removeClass('active');
               menu.find('.arrow').hide();
            }
         });

         this._menu.find('.menuButton').find('.arrow').hide();
      },

      _contains: function(parentKey, childKey, data) {
         //console.log('does ' + parentKey + ' contain ' + childKey + '?' );
         var container = this._getPropertyInObject(parentKey, this.options.menu, '   ');
         //console.log('   container is ');
         //console.log(container)
         var child = this._getPropertyInObject(childKey, container, '   ');

         //console.log(child != null);
         return child != null;
      },

      _getPropertyInObject: function(key, obj, indent) {
         //console.log(indent + 'looking for ' + key + ' in ' + obj);

         for (var prop in obj) {
            //console.log(indent + prop + ': ' + obj[prop]);
            if (prop == key) {
               //console.log(indent + 'found ' + key + '!!!');
               return obj[prop];
            } else if (obj[prop] instanceof Object && typeof obj[prop] != 'function') {
               result = this._getPropertyInObject(key, obj[prop], indent + '   ');
               if (result) {
                  return result;
               }
            }
         }

         return null;
      },

      _hideAllMenus: function() {
         this.element.find('.menu').hide();
         this.element.find('.clickable').removeClass('active');
         this.element.find('.arrow').hide();
         this._activeMenu = null;
      },

      destroy: function () {
         //this._clearItems();
         this.element.html("");
      }

      /*
      _setOption: function (option, value) {
         $.Widget.prototype._setOption.apply(this, arguments);
      }
      */

   });

})(jQuery);