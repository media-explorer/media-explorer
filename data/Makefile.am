SUBDIRS = appicons

extradir = $(pkgdatadir)/common
dist_extra_DATA = \
	folder-tile.png \
	thumb-image.png \
	thumb-video.png \
	thumb-music.png \
	$(NULL)

jsondir = $(pkgdatadir)/json
dist_json_DATA = \
	json/media-controls.json	\
	json/slide-show.json		\
	json/info-bar.json		\
	json/info-panel-full.json	\
	json/info-panel-simple.json	\
	$(NULL)

if HAVE_LIRC
dist_extra_DATA += mex-lircrc
endif

styledir = $(pkgdatadir)/common/style
dist_style_DATA = \
		  style/details-actions-separator.png \
		  style/details-bg.png \
		  style/grid-content-bg.png \
		  style/harderror-bg.png \
		  style/highlight-bg.png \
		  style/item-title-bg.png \
		  style/label-bg.png \
		  style/panel-header-bg.png \
		  style/panel-highlight.png \
		  style/panel-shadow.png \
		  style/queue-spinner.png \
		  style/queue-spinner-highlight.png \
		  style/scroll-bg.png \
		  style/scroll-bar.png \
		  style/thumb-noContent.png \
		  style/spinner.png \
		  style.css

if ENABLE_IDLE_VIDEO
dist_style_DATA +=style/background-loop.mkv
endif

shellstyledir = $(pkgdatadir)/shell/style
dist_shellstyle_DATA =		\
	shell/style.css		\
	shell/folder-tile-up.png \
	shell/main-bg.png	\
	shell/panel-header-grid-bg.png \
	$(NULL)

videocontrolsstyledir = $(pkgdatadir)/common/style/video-controls
dist_videocontrolsstyle_DATA = \
		style/video-controls/header-vr.png \
		style/video-controls/highlight.png \
		style/video-controls/icon-play-highlight.png \
		style/video-controls/icon-play.png \
		style/video-controls/icon-pause-highlight.png \
		style/video-controls/icon-pause.png \
		style/video-controls/icon-stop-highlight.png \
		style/video-controls/icon-stop.png \
		style/video-controls/main-bg.png \
		style/video-controls/panel-bg.png \
		style/video-controls/timeline-bg.png \
		style/video-controls/timeline-buffer.png \
		style/video-controls/timeline-highlight.png \
		style/video-controls/timeline-highlight-white.png \
		style/video-controls/timeline-playhead.png

slideshowstyledir = $(pkgdatadir)/common/style/slide-show
dist_slideshowstyle_DATA = \
		style/slide-show/highlight-bg-white.png \
		style/slide-show/icon-actualsize-highlight.png \
		style/slide-show/icon-actualsize.png \
		style/slide-show/icon-fullscreen-highlight.png \
		style/slide-show/icon-fullscreen.png \
		style/slide-show/icon-rotate-highlight.png \
		style/slide-show/icon-rotate.png

infobarstyledir = $(pkgdatadir)/common/style/info-bar
dist_infobarstyle_DATA = \
			style/info-bar/graphic-network.png \
			style/info-bar/graphic-rebinder.png

volumecontrolstyledir = $(pkgdatadir)/common/style/volume-control
dist_volumecontrolstyle_DATA = \
			style/volume-control/volume-0.png \
			style/volume-control/volume-1.png \
			style/volume-control/volume-2.png \
			style/volume-control/volume-3.png \
			style/volume-control/volume-4.png \
			style/volume-control/volume-5.png \
			style/volume-control/volume-6.png \
			style/volume-control/volume-7.png \
			style/volume-control/volume-8.png \
			style/volume-control/volume-9.png \
			style/volume-control/volume-10.png




icondir = $(styledir)/icons/mex
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
