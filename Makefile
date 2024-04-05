# @author   clemedon (Clément Vidon)
####################################### BEG_3 ####

NAME        := netOcd_mongoose_webServer

#------------------------------------------------#
#   INGREDIENTS                                  #
#------------------------------------------------#
# SRC_DIR   source directory
# OBJ_DIR   object directory
# SRCS      source files
# OBJS      object files
#
# CC        compiler
# CFLAGS    compiler flags
# CPPFLAGS  preprocessor flags

SRC_DIR     := src
OBJ_DIR     := obj
SRCS        := \
	net.c           \
	main.c          \
	get_cpuage.c    \
	sdl_thread_lib/netocd_config_json.c \
	sdl_thread_lib/real_function.c \
	sdl_thread_lib/sdl_thread_bk.c \
	sdl_thread_lib/netbps_fork.c \
	sdl_thread_lib/mgcf_openocd.c \
	3thpart/mongoose/mongoose.c  \
	3thpart/mongoose/cJSON.c   

	

SRCS        := $(SRCS:%=$(SRC_DIR)/%)
OBJS        := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

INC_DIR     := src/3thpart/mongoose  src/sdl_thread_lib
CC          := gcc
# CFLAGS      := -Wall -Wextra -Werror -std=gnu99 
LKFLAGS     := -lSDL2
CFLAGS      := -Wall -Wextra -std=gnu99
CPPFLAGS    := -I $(INC_DIR)

#------------------------------------------------#
#   UTENSILS                                     #
#------------------------------------------------#
# RM        cleaning command
# MAKE      make command

RM          := rm --force
MAKE        := make --no-print-directory

#------------------------------------------------#
#   RECIPES                                      #
#------------------------------------------------#
# all       default goal
# %.o       compilation .c -> .o
# $(NAME)   linking .o -> binary
# clean     remove .o
# fclean    remove .o + binary
# re        remake default goal

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $^ -o $@ $(LKFLAGS)
	echo "CREATED $(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	-[ ! -d $(@D) ] && mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	echo "CREATED $@"

clean:
	$(RM) --recursive $(OBJ_DIR)
	echo "rm --recursive " $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)
	echo "rm " $(NAME)


re:
	$(MAKE) fclean
	$(MAKE) all

#------------------------------------------------#
#   SPEC                                         #
#------------------------------------------------#

.PHONY: clean fclean re
.SILENT:

####################################### END_3 ####
