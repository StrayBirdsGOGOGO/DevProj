#######################
# Local Configuration #
#######################

CC        = g++
GEN_FLAGS    = -ggdb -Wall
GEN_INCLUDES = -I../include -I/home/wyt/soft/log4cxx/include
GEN_LIBS     = -ldl -mt -lpthread -lgcc_s -llog4cxx -L/home/wyt/soft/log4cxx/lib

CXXFLAGS = $(GEN_FLAGS)
INCLUDES = $(GEN_INCLUDES)
LIBS     = $(GEN_LIBS)


SRC       = command.cpp cyclicqueue.cpp comm.cpp control.cpp  g.cpp hwsqueue.cpp main.cpp  message.cpp queue.cpp appmonitor.cpp sugwserver.cpp md5.cpp transmit.cpp zxclient.cpp zxcomm.cpp cdrmessage.cpp
 
OBJ_FILE  = $(SRC:.cpp=.o)
EXE_FILE  = ../bin/sigparser

${EXE_FILE}: ${OBJ_FILE}
	${CC} ${CXXFLAGS}  -o ${EXE_FILE}  ${OBJ_FILE}  ${LIBS} 

.cpp.o:
	$(CC) -c $(CXXFLAGS) $(INCLUDES)  $<

clean:
	rm -f ${OBJ_FILE} ${EXE_FILE}

