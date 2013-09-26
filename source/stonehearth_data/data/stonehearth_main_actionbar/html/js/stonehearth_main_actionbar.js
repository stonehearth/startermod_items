var stonehearthMainActionbarElement = null;

$(document).ready(function(){

   $(top).on('show_main_actionbar.radiant', function (_, data) {
      stonehearthMainActionbarElement.show();
   });

   $(top).on('hide_main_actionbar.radiant', function (_, e) {
      stonehearthMainActionbarElement.hide();
   });

});

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

      var imagePath = '/stonehearth_main_actionbar/html/images/'
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
                        $(top).trigger('create_stockpile.radiant');
                     }
                  },
                  workshop: {
                     name: 'Workshop',
                     icon: imagePath + 'workshop.png',
                     hotkey: 'w',
                     items: {
                        carpenter: {
                           name: 'Carpenter',
                           icon: imagePath + 'carpenter.png',
                           hotkey: 'c',
                           click: function () {
                              $(top).trigger('build_workshop.stonehearth', {
                                 uri: '/stonehearth_carpenter_class/job_info.json'
                              });
                           }
                        }
                     }
                  },
                  building: {
                     name: 'Building',
                     icon: imagePath + 'building.png',
                     hotkey: 'b'
                  },
                  placement: {
                     name: 'Place Item',
                     icon: imagePath + 'place_item.png',
                     hotkey: 'p',
                     click: function () {
                        $(top).trigger('placement_menu.radiant');
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

