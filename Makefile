PROG=	scrollman
SRCS=	main.c actions.c config.c events.c layout.c x11.c
NOMAN=	yes
BINDIR=	/usr/local/bin

CFLAGS+=	-Wall -I/usr/X11R6/include
LDADD+=		-L/usr/X11R6/lib -lX11

.include <bsd.prog.mk>
