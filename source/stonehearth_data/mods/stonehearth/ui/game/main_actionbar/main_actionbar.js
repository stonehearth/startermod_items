var stonehearthMainActionbarElement = null;

App.StonehearthMainActionbarView = App.View.extend({
   templateName: 'stonehearthMainActionbar',

   components: {

   },

   init: function() {
      this._super();
      // xxx, grab the initial button state from some server-side component
   },

   didInsertElement: function() {
      var element = $('#mainActionbar');
      stonehearthMainActionbarElement = element;

      var imagePath = '/stonehearth/ui/game/main_actionbar/images/'
      element.boxmenu( {
         menu: {
            build: {
               name: 'Build',
               icon: imagePath + 'build.png',
               hotkey: 'b',
               items: {
                  stockpile: {
                     name: 'Stockpile',
                     icon: imagePath + 'stockpile.png',
                     hotkey: 's',
                     click: function () {
                        $(top).trigger('radiant_create_stockpile');
                     }
                  },
                  workshop: {
                     name: 'Workshop',
                     icon: imagePath + 'workshop.png',
                     hotkey: 'w',
                     items: {
                        carpenter: {
                           name: 'Carpenterrrr',
                           icon: imagePath + 'carpenter.png',
                           hotkey: 'c',
                           click: function () {
                              $(top).trigger('build_workshop.stonehearth', {
                                 uri: '/stonehearth/entities/professions/carpenter/profession_description.json'
                              });
                           }
                        },
                        weaver: {
                           name: 'Weaver',
                           icon: imagePath + 'weaver.png',
                           hotkey: 'w',
                           click: function () {
                              $(top).trigger('build_workshop.stonehearth', {
                                 uri: '/stonehearth/entities/professions/weaver/profession_description.json'
                              });
                           }
                        }
                     }
                  },
                  building: {
                     name: 'Building',
                     icon: imagePath + 'building.png',
                     hotkey: 'v',
                     items: {
                        wall_tool: {
                           name: 'Build Wall!',
                           icon: imagePath + 'stockpile.png',
                           hotkey: 'c',
                           click: function () {
                              $(top).trigger('radiant_create_wall');
                           }
                        }
                        }
                     }
                  },
                  placement: {
                     name: 'Place Item',
                     icon: imagePath + 'place_item.png',
                     hotkey: 'p',
                     click: function () {
                        $(top).trigger('radiant_show_placement_menu');
                     }
                  }
               }
            },
            harvest: {
               name: 'Harvest',
               icon: imagePath + 'harvest.png',
               hotkey: 'h',
               styleClass: 'harvestMenu',
               items: {
                  chop: {
                     name: 'Chop trees',
                     icon: imagePath + 'chop.png',
                     hotkey: 'c'
                  }
               }
            }
         }
      });
   },

});

