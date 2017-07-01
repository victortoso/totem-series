CC=gcc
LIBS=`pkg-config --libs grilo-0.3 gtk+-3.0 grilo-net-0.3 emeus-1.0`
CFLAGS= `pkg-config --cflags grilo-0.3 gtk+-3.0 grilo-net-0.3 emeus-1.0`
CFLAGS+= -Wall -g -DON_DEVELOPMENT -DG_LOG_DOMAIN=\"totem\"
TARGET=bin
CCRESOURCES=glib-compile-resources

all:
	$(CCRESOURCES) totem-video-summary.gresource.xml --target=tvsresources.h --c-name _totem_video_summary --generate-header
	$(CCRESOURCES) totem-video-summary.gresource.xml --target=tvsresources.c --c-name _totem_video_summary --generate-source
	$(CC) $(CFLAGS) -c tvsresources.c $(LIBS)
	$(CC) $(CFLAGS) -c totem-episode-view.c $(LIBS)
	$(CC) $(CFLAGS) -c totem-series-summary.c $(LIBS)
	$(CC) $(CFLAGS) -c totem-series-view.c $(LIBS)
	$(CC) $(CFLAGS) sample.c totem-episode-view.o totem-series-summary.o totem-series-view.o tvsresources.o -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) totem-episode-view.o totem-series-summary.o totem-series-view.o tvsresources.*
