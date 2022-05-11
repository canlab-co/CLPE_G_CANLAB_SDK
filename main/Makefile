### Makefile for the Create CanLab CLPE Client API ###

CC=g++

CFLAGS	= -Wall

DFLAGS	+=

GSTREMER_PKG	= gstreamer-1.0
GSTREAMER_CFG	= `pkg-config $(GSTREMER_PKG) --cflags`
CDFLAGS	= $(CFLAGS) $(DFLAGS) $(INCLUDE) $(GSTREAMER_CFG)

LIBS	=
GSTREAMER_LIB	= `pkg-config $(GSTREMER_PKG) --libs`
GSTREMER_APP_PKG	= gstreamer-app-1.0
GSTREAMER_LIB	+= `pkg-config $(GSTREMER_APP_PKG) --libs`
LIBS	+= $(GSTREAMER_LIB)

LDFLAGS	+= $(LIBS)

EXEC=ClpeApi_Example_Main
OBJS=ClpeApi_Example_Main.o

$(EXEC): $(OBJS)
	 	 $(CC) $(CDFLAGS) -o $@ $(OBJS) -L../lib -lclpe $(LDFLAGS) `pkg-config $(GSTREMER_PKG) --cflags`

%.o: %.cpp
	 $(CC) $(CDFLAGS) -c $< -o $@ -I../include $(LDFLAGS) `pkg-config $(GSTREMER_PKG) --cflags`

clean:
		rm -f $(OBJS) $(EXEC) 
