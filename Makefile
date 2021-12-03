CXXFLAGS += -std=c++20 $(shell pkg-config --cflags fmt hidapi-libusb)
LDLIBS += $(shell pkg-config --libs fmt hidapi-libusb)

all: single_color rainbow test
single_color: single_color.cpp x50q.hpp hidapi.hpp
rainbow: rainbow.cpp x50q.hpp hidapi.hpp
test: test.cpp x50q.hpp hidapi.hpp
clean:
	rm -f single_color rainbow test
