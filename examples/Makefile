CPPFLAGS := -MMD
CFLAGS := -std=gnu99 -Wall -Wextra -Werror=uninitialized -O0 -g

sources := writer.c
objects := $(sources:.c=.o)
depends := $(sources:.c=.d)

# libtmio must be first bc it overloads some of the MPI symbols
writer: CPPFLAGS += -I ../include -L ../build
writer: LDLIBS += -ltmio
writer: CPPFLAGS += `pkg-config --cflags mpi`
writer: LDLIBS += `pkg-config --libs mpi`

-include $(depends)
