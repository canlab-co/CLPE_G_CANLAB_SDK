### Makefile for the Create CanLab CLPE Client API ###

CC=g++
AR=ar

CFLAGS	= -Wall

DFLAGS	+=

ARFLAGS	= crv

GSTREMER_PKG	= gstreamer-1.0
GSTREAMER_CFG	= `pkg-config $(GSTREMER_PKG) --cflags`
CDFLAGS	= $(CFLAGS) $(DFLAGS) $(INCLUDE) $(GSTREAMER_CFG)

LIBS	=
GSTREAMER_LIB	= `pkg-config $(GSTREMER_PKG) --libs`
GSTREMER_APP_PKG	= gstreamer-app-1.0
GSTREAMER_LIB	+= `pkg-config $(GSTREMER_APP_PKG) --libs`
LIBS	+= $(GSTREAMER_LIB)

LDFLAGS	+= $(LIBS)

LIB=libclpe.a
OBJS=ClpeSocket.o ClpeClientApi.o ClpeStream.o

$(LIB): $(OBJS)
		$(AR) rv $@ $(OBJS)

%.o: %.cpp
	 $(CC) $(CDFLAGS) -c -I../include $< -o $@ $(LDFLAGS) `pkg-config $(GSTREMER_PKG) --cflags`

clean:
		rm -f $(OBJS) $(LIB)   
