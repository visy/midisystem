CXX=g++
CFLAGS = -g -O3 -W -Wall
LDFLAGS = -s
AR=ar
RANLIB=ranlib

DEMO_CFLAGS = -fpermissive

OBJPATH=obj/


###
### Depency library definitions
###
BOOST_CFLAGS=-I/usr/include/
BOOST_LDFLAGS=-lboost_thread -lboost_system -lpthread
#/usr/lib/libboost_thread.a /usr/lib/libboost_system.a

FREETYPE_CFLAGS=`pkg-config freetype2 --cflags`
FREETYPE_LDFLAGS=`pkg-config freetype2 --libs`

GLEW_CFLAGS=`pkg-config glew --cflags`
GLEW_LDFLAGS=`pkg-config glew --libs`

GLUT_CFLAGS=-I/usr/include/GL/
GLUT_LDFLAGS=-lglut
#/usr/lib/i386-linux-gnu/libglut.a

OPENGL_CFLAGS=`pkg-config gl glu --cflags`
OPENGL_LDFLAGS=`pkg-config gl glu --libs`


###
### Included in source tree
###
BASS_CFLAGS=-Ilibbass/
BASS_LDFLAGS=-L./lib/ -lbass

ASSIMP=assimp--3.0.1270-source-only/
ASSIMP_CFLAGS=-I$(ASSIMP)include/
ASSIMP_LDFLAGS=-L./lib/ -lassimp


LIBOGGPLAYER=liboggplayer-src/
LIBOGGPLAYER_CFLAGS=-I$(LIBOGGPLAYER)include/
#LIBOGGPLAYER_LDFLAGS=`pkg-config --libs ogg vorbis theoradec`
LIBOGGPLAYER_LDFLAGS=`pkg-config --libs ogg theoradec`
LIBOGGPLAYER_A=$(OBJPATH)liboggplayer.a


DEMO_CFLAGS +=  $(BASS_CFLAGS) $(ASSIMP_CFLAGS) \
		$(LIBOGGPLAYER_CFLAGS) $(FREETYPE_CFLAGS) \
		$(GLEW_CFLAGS) $(GLUT_CFLAGS) \
		$(FREETYPE_CFLAGS) $(OPENGL_CFLAGS)

DEMO_LDFLAGS = $(BASS_LDFLAGS) $(ASSIMP_LDFLAGS) \
		$(LIBOGGPLAYER_A) $(FREETYPE_LDFLAGS) \
		$(GLEW_LDFLAGS) $(GLUT_LDFLAGS) \
		$(FREETYPE_LDFLAGS) $(BOOST_LDFLAGS) \
		$(LIBOGGPLAYER_LDFLAGS) $(OPENGL_LDFLAGS)



###
### Source files etc
###
TARGETS=demo.bin


DEMO_OBJS=platform.o vertex-attribute.o vertex-buffer.o \
	texture-atlas.o texture-font.o mat4.o \
	shader.o vector.o midifile.o midiutil.o main.o

LIBOGGPLAYER_OBJS= \
	oggplayer.o open_close.o play.o \
	util.o
	

###
### Main targets
###
all: $(TARGETS)

$(OBJPATH)%.o: $(LIBOGGPLAYER)src/%.cpp $(LIBOGGPLAYER)src/%.hpp
	@echo " CXX $+"
	@$(CXX) $(CFLAGS) -c -o $@ $< $(LIBOGGPLAYER_CFLAGS) $(BOOST_CFLAGS)

$(OBJPATH)%.o: $(LIBOGGPLAYER)src/%.cpp
	@echo " CXX $+"
	@$(CXX) $(CFLAGS) -c -o $@ $< $(LIBOGGPLAYER_CFLAGS) $(BOOST_CFLAGS)


$(OBJPATH)%.o: src/%.c src/%.h
	@echo " CXX $+"
	@$(CXX) $(CFLAGS) -c -o $@ $< $(DEMO_CFLAGS)

$(OBJPATH)%.o: src/%.c
	@echo " CXX $+"
	@$(CXX) $(CFLAGS) -c -o $@ $< $(DEMO_CFLAGS)


$(OBJPATH)main.o: src/main.c src/stb_image.c src/ggets.c src/ggets.h
	@echo " CXX $+"
	@$(CXX) $(CFLAGS) -c -o $@ $< $(DEMO_CFLAGS)


$(LIBOGGPLAYER_A): $(addprefix $(OBJPATH),$(LIBOGGPLAYER_OBJS))
	@echo " AR $@ $+"
	@$(AR) cru $@ $+
	@echo " RANLIB $@"
	@$(RANLIB) $@


demo.bin: $(addprefix $(OBJPATH),$(DEMO_OBJS)) $(LIBOGGPLAYER_A)
	@echo " LINK $@ $+"
	@$(CXX) $(CFLAGS) $(DEMO_CFLAGS) -o $@ $+ $(DEMO_LDFLAGS)


###
### Special targets
###
clean:
	$(RM) $(TARGETS) $(OBJPATH)*.o $(LIBOGGPLAYER_A)

distclean: clean
	$(RM) -fr $(ASSIMP) libbass/


build-assimp:
	unzip assimp--3.0.1270-source-only.zip && \
	cd $(ASSIMP) && \
	cmake . -DENABLE_BOOST_WORKAROUND=ON && \
	make -j4

build-bass:
	mkdir libbass/ && cd libbass && unzip ../bass24-linux.zip

libs32: $(ASSIMP)lib/libassimp.so libbass/libbass.so
	cp -d -p $(addsuffix *,$+) lib/

libs64: $(ASSIMP)lib/libassimp.so libbass/x64/libbass.so
	cp -d -p $(addsuffix *,$+) lib/

build32: build-assimp build-bass libs32 demo.bin

build64: build-assimp build-bass libs64 demo.bin
