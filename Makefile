CXXFLAGS += -std=c++20 $(shell pkg-config --cflags fmt hidapi-hidraw libusb-1.0) -Iinclude
LDLIBS += $(shell pkg-config --libs fmt hidapi-hidraw libusb-1.0)

all: demo/single_color demo/rainbow demo/test
demo/single_color: demo/single_color.cpp include/x50q.hpp include/hidapi.hpp
demo/rainbow: demo/rainbow.cpp include/x50q.hpp include/hidapi.hpp
demo/test: demo/test.cpp include/x50q.hpp include/hidapi.hpp
clean:
	rm -f demo/single_color demo/rainbow demo/test
