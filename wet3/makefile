# 046267 Computer Architecture - HW #3
# makefile for test environment

all: dflow_calc

# Environment for C
CC = gcc
CFLAGS = -std=c99 -Wall
# Environment for C++
CXX = g++
CXXFLAGS = -std=c++11 -Wall

ifeq ($(DEBUG),1)
  CFLAGS += -g -O0
  CXXFLAGS += -g -O0
else
  CFLAGS += -O2
  CXXFLAGS += -O2
endif

# Automatically detect whether the bp is C or C++
# Must have either dflow_calc.c or dflow_calc.cpp - NOT both
SRC_DFLOW = $(wildcard dflow_calc.c dflow_calc.cpp)
SRC_GIVEN = dflow_main.c
EXTRA_DEPS = dflow_calc.h

OBJ_GIVEN = $(patsubst %.c,%.o,$(SRC_GIVEN))
OBJ_DFLOW = dflow_calc.o
OBJ = $(OBJ_GIVEN) $(OBJ_DFLOW)

#$(info OBJ=$(OBJ))


ifeq ($(SRC_DFLOW),dflow_calc.c)
dflow_calc: $(OBJ)
	$(CC) -o $@ $(OBJ)

dflow_calc.o: dflow_calc.c dflow_calc.h
	$(CC) -c $(CFLAGS) -o $@ $<

else
dflow_calc: $(OBJ)
	$(CXX) -o $@ $(OBJ)

dflow_calc.o: dflow_calc.cpp dflow_calc.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<
endif

$(OBJ_GIVEN): %.o: %.c dflow_calc.h
	$(CC) -c $(CFLAGS) -o $@ $<


.PHONY: clean
clean:
	rm -f dflow_calc $(OBJ)
