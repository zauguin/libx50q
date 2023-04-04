CXXFLAGS += -std=c++20 $(shell pkg-config --cflags fmt hidapi-hidraw libusb-1.0)
LDLIBS += $(shell pkg-config --libs fmt hidapi-hidraw libusb-1.0)

all: static_rainbow single_color rainbow test
single_color: single_color.cpp x50q.hpp hidapi.hpp
static_rainbow: static_rainbow.cpp x50q.hpp hidapi.hpp
rainbow: rainbow.cpp x50q.hpp hidapi.hpp
test: test.cpp x50q.hpp hidapi.hpp
clean:
	rm -f single_color rainbow test static_rainbow
