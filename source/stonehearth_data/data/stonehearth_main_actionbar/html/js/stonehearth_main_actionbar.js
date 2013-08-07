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
               items: {
                  stockpile: {
                     name: 'Stockpile',
                     icon: imagePath + 'stockpile.png'
                  },
                  workshop: {
                     name: 'Workshop',
                     icon: imagePath + 'workshop.png',
                     items: {
                        carpenter: {
                           name: 'Carpenter',
                           icon: imagePath + 'carpenter.png'
                        }
                     }
                  },
                  building: {
                     name: 'Building',
                     icon: imagePath + 'building.png'
                  }
               }
            },
            harvest: {
               name: 'Harvest',
               icon: imagePath + 'harvest.png',
               items: {
                  chop: {
                     name: 'Chop trees',
                     icon: imagePath + 'chop.png'
                  }
               }
            }
         }
      });

   },

});

