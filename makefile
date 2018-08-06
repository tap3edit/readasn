CC = gcc

PKG_VER = 0.02

SRC  = readasn.c
SRC += tagnames.c

OBJ  = $(SRC:.c=.o)

READASN = readasn
PKG_NAME = $(READASN)-$(PKG_VER).zip


CFLAGS = -Wall -g

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(READASN)
	
$(READASN):	$(OBJ)
	$(CC) $(OBJ) -o $@

# readasn.o: readasn.c readasn.h
#  
# tagids.o: tagids.c readasn.h

PKG_TMP_DIR = tmp
PKG_BASE_DIR = readasn

PKG_SRC = $(SRC) \
		  $(READASN) \
		  readasn.h \
		  makefile \
		  ChangeLog

pkg: make_dir \
	copy_src \
	tar_module \
	rm_dir

make_dir:
	@mkdir -p $(PKG_TMP_DIR)/$(PKG_BASE_DIR)/.

copy_src:
	@cp $(PKG_SRC) $(PKG_TMP_DIR)/$(PKG_BASE_DIR)/.

tar_module:
	cd $(PKG_TMP_DIR); zip -r ../$(PKG_NAME) $(PKG_BASE_DIR)


rm_dir: 
	rm -rf $(PKG_TMP_DIR)
clean:
	rm -rf *.o $(PKG_NAME) $(READASN)
