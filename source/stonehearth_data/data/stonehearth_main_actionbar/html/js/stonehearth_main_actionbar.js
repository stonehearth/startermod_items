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
                     hotkey: 's'
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
                              $(top).trigger('create_workshop.radiant', {
                                 workbench: '/stonehearth_carpenter_class/entities/carpenter_workbench'
                              });  
                           }
                        }
                     }
                  },
                  building: {
                     name: 'Building',
                     icon: imagePath + 'building.png',
                     hotkey: 'b'
                  }
               }
            },
            harvest: {
               name: 'Harvest',
               icon: imagePath + 'harvest.png',
               hotkey: 'h',
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

