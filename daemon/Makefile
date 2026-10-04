CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2

all: token_bucket_daemon

token_bucket_daemon: main.cpp ../driver/tbq_ioctl.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm -f token_bucket_daemon
