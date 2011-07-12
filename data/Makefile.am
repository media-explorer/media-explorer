SUBDIRS = appicons

jsondir = $(pkgdatadir)/json
dist_json_DATA = \
	json/media-controls.json	\
	json/slide-show.json		\
	json/info-bar.json		\
	json/info-panel-full.json	\
	json/info-panel-simple.json	\
	$(NULL)

if HAVE_LIRC
extradir = $(pkgdatadir)/common
dist_extra_DATA = mex-lircrc
endif

styledir = $(pkgdatadir)/style
dist_style_DATA = \
	style/controls-bg.png \
	style/details-actions-separator.png \
	style/details-bg.png \
	style/folder-tile.png \
	style/folder-tile-up.png \
	style/graphic-network.png \
	style/graphic-rebinder.png \
	style/grid-content-bg.png \
	style/harderror-bg.png \
	style/header-vr.png \
	style/highlight-bg.png \
	style/highlight-bg-white.png \
	style/highlight.png \
	style/icon-actualsize-highlight.png \
	style/icon-actualsize.png \
	style/icon-fullscreen-highlight.png \
	style/icon-fullscreen.png \
	style/icon-pause-highlight.png \
	style/icon-pause.png \
	style/icon-play-highlight.png \
	style/icon-play.png \
	style/icon-rotate-highlight.png \
	style/icon-rotate.png \
	style/icon-stop-highlight.png \
	style/icon-stop.png \
	style/item-title-bg.png \
	style/label-bg.png \
	style/main-bg.png \
	style/panel-bg.png \
	style/panel-header-bg.png \
	style/panel-header-grid-bg.png \
	style/panel-highlight.png \
	style/panel-shadow.png \
	style/queue-spinner-highlight.png \
	style/queue-spinner.png \
	style/scroll-bar.png \
	style/scroll-bg.png \
	style/spinner.png \
	style/style.css \
	style/thumb-bg.png \
	style/thumb-image.png \
	style/thumb-music.png \
	style/thumb-no-content.png \
	style/thumb-tv.png \
	style/thumb-video.png \
	style/timeline-bg.png \
	style/timeline-buffer.png \
	style/timeline-highlight.png \
	style/timeline-highlight-white.png \
	style/timeline-playhead.png \
	style/volume-0.png \
	style/volume-10.png \
	style/volume-1.png \
	style/volume-2.png \
	style/volume-3.png \
	style/volume-4.png \
	style/volume-5.png \
	style/volume-6.png \
	style/volume-7.png \
	style/volume-8.png \
	style/volume-9.png \
	$(NULL)



if ENABLE_IDLE_VIDEO
dist_style_DATA +=style/background-loop.mkv
endif

icondir = $(pkgdatadir)/icons/mex
dist_icon_DATA = \
		 icons/index.theme

icon_22x22dir = $(icondir)/22x22
dist_icon_22x22_DATA = \
		 icons/icon-info-highlight.png

icon_26x26dir = $(icondir)/26x26
dist_icon_26x26_DATA = \
		 icons/icon-notifications.png \
		 icons/icon-power.png \
		 icons/icon-power-highlight.png \
		 icons/icon-settings.png \
		 icons/icon-settings-highlight.png \
		 icons/icon-checkbox-off.png \
		 icons/icon-checkbox-off-highlight.png \
		 icons/icon-checkbox-on.png \
		 icons/icon-checkbox-on-highlight.png \
		 icons/media-addtoqueue-mex.png \
		 icons/media-addtoqueue-mex-highlight.png \
		 icons/media-removefromqueue-mex.png \
		 icons/media-removefromqueue-mex-highlight.png \
		 icons/media-watch-mex.png \
		 icons/media-watch-mex-highlight.png \
		 icons/media-watch-from-beginning-mex.png \
		 icons/media-watch-from-beginning-mex-highlight.png \
		 icons/x-mex-app-launch-mex.png \
		 icons/x-mex-app-launch-mex-highlight.png \
		 icons/stock_left-mex-highlight.png \
		 icons/stock_left-mex.png \
		 icons/stock_right-mex-highlight.png \
		 icons/stock_right-mex.png \
		 icons/icon-panelheader-apps.png \
		 icons/icon-panelheader-apps-highlight.png \
		 icons/icon-panelheader-computer-highlight.png \
		 icons/icon-panelheader-computer.png \
		 icons/icon-panelheader-photos.png \
		 icons/icon-panelheader-photos-highlight.png \
		 icons/icon-panelheader-recommended.png \
		 icons/icon-panelheader-recommended-highlight.png \
		 icons/icon-panelheader-recorded.png \
		 icons/icon-panelheader-recorded-highlight.png \
		 icons/icon-panelheader-search.png \
		 icons/icon-panelheader-search-highlight.png \
		 icons/icon-panelheader-tv.png \
		 icons/icon-panelheader-tv-highlight.png \
		 icons/icon-panelheader-music.png \
		 icons/icon-panelheader-music-highlight.png \
		 icons/icon-panelheader-queue-highlight.png \
		 icons/icon-panelheader-queue.png \
		 icons/icon-panelheader-usb-highlight.png \
		 icons/icon-panelheader-usb.png \
		 icons/icon-panelheader-videos.png \
		 icons/icon-panelheader-videos-highlight.png

if ENABLE_REBINDER
rebinderdir = $(pkgdatadir)/rebinder
dist_rebinder_DATA = 				\
	rebinder/back.json			\
	rebinder/home.json			\
	rebinder/info.json			\
	rebinder/welcome.json			\
	rebinder/done.json			\
	rebinder/setup.json			\
	rebinder/error-back.json		\
	rebinder/error-home.json		\
	rebinder/error-info.json		\
	rebinder/button-bg.png			\
	rebinder/button-bg-highlight.png	\
	rebinder/frame-bg.png			\
	rebinder/icon-back.png			\
	rebinder/icon-home.png			\
	rebinder/icon-info.png			\
	rebinder/remote.png			\
	rebinder/remote-back.png		\
	rebinder/remote-back-error.png		\
	rebinder/remote-back-hl.png		\
	rebinder/remote-home.png		\
	rebinder/remote-home-error.png		\
	rebinder/remote-home-hl.png		\
	rebinder/remote-info.png		\
	rebinder/remote-info-error.png		\
	rebinder/remote-info-hl.png		\
	rebinder/style.css			\
	$(NULL)
endif

if ENABLE_NETWORKS
networksdir = $(pkgdatadir)/networks
dist_networks_DATA =				\
	networks/background.png			\
	networks/content-box.png		\
	networks/service-list.png		\
	networks/icon-tick.png			\
	networks/icon-tick-focus.png		\
	networks/icon-info.png			\
	networks/icon-lock.png			\
	networks/icon-lock-focus.png		\
	networks/icon-wifi-0.png		\
	networks/icon-wifi-0-focus.png		\
	networks/icon-wifi-1.png		\
	networks/icon-wifi-1-focus.png		\
	networks/icon-wifi-2.png		\
	networks/icon-wifi-2-focus.png		\
	networks/icon-wifi-3.png		\
	networks/icon-wifi-3-focus.png		\
	networks/icon-wired.png			\
	networks/icon-wired-focus.png		\
	networks/mex-networks.css		\
	$(NULL)
endif

applicationsdir = $(datadir)/applications
dist_applications_DATA =			\
	mex.desktop			\
	$(NULL)

if ENABLE_REBINDER
dist_applications_DATA += mex-rebinder-config.desktop
endif

if ENABLE_NETWORKS
dist_applications_DATA += mex-networks.desktop
endif

griloconfdir = $(datadir)/$(PACKAGE)
dist_griloconf_DATA = grilo-system.conf
