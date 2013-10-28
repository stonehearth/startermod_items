;( function( $, window, undefined ) {

	'use strict';

	$.ToggleGrid = function( options, element ) {
		this.$el = $( element );
		this._init( options );
	};

	// the options
	$.ToggleGrid.defaults = {
		radios : false,
		onItemClick : function( el, name ) { return false; }
	};

	$.ToggleGrid.prototype = {
		_init : function( options ) {

			// options
			this.options = $.extend( true, {}, $.ToggleGrid.defaults, options );
			// cache some elements and initialize some variables
			this._config();
			this._initEvents();

		},
		_config : function() {
			this.$el.addClass('toggleGrid');
			this.$items = this.$el.find('.toggleButton');
		},

		_initEvents : function() {
			var self = this;
			this.$items.on( 'click.toggleGrid', function( event ) {
				event.stopPropagation();

				if (self.options.radios) {
					self.$items.removeClass('on');
				}

				var $item = $(this)

				if ($item.hasClass('on')) {
					$item.removeClass('on')
				} else {
					$item.addClass('on')
				}

				self.options.onItemClick( $item, event );
			});
		}
	};

	$.fn.togglegrid = function( options ) {
		this.each(function() {	
			var instance = $.data( this, 'togglegrid' );
			if ( instance ) {
				instance._init();
			}
			else {
				instance = $.data( this, 'togglegrid', new $.ToggleGrid( options, this ) );
			}
		});
		return this;
	};

} )( jQuery, window );