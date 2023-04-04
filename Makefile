CXXFLAGS += -std=c++20 $(shell pkg-config --cflags fmt hidapi-hidraw libusb-1.0) -Iinclude
LDLIBS += $(shell pkg-config --libs fmt hidapi-hidraw libusb-1.0)

.PHONY: all demo profile clean clean.demo clean.profile
all: demo profile
clean: clean.demo clean.profile

demo: demo/single_color demo/rainbow demo/test
demo/single_color: demo/single_color.cpp include/x50q.hpp include/hidapi.hpp
demo/rainbow: demo/rainbow.cpp include/x50q.hpp include/hidapi.hpp
demo/test: demo/test.cpp include/x50q.hpp include/hidapi.hpp

clean.demo:
	rm -f demo/single_color demo/rainbow demo/test

profile: profile/apply_profile profile/edit_profile
profile/apply_profile: profile/apply_profile.cpp include/x50q.hpp include/hidapi.hpp
profile/edit_profile: profile/edit_profile.cpp include/x50q.hpp include/hidapi.hpp
clean.profile:
	rm -f profile/apply_profile profile/edit_profile
