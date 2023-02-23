#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "json.hpp"
extern "C" {
  #include <dfu.h>
  #include <dfu_util.h>
  // This is missing from dfu_util.h
  char *get_path(libusb_device *dev);
  #include <portable.h>
  #include <libusb.h>

  // Those are cmd-line arguments for dfu-util, we provide
  // the default value for them
  int match_devnum = -1;
  int verbose = 0;
  char *match_path = NULL;
  int match_vendor = -1;
  int match_product = -1;
  int match_vendor_dfu = -1;
  int match_product_dfu = -1;
  int match_config_index = -1;
  int match_iface_index = -1;
  int match_iface_alt_index = -1;
  const char *match_iface_alt_name = NULL;
  const char *match_serial = NULL;
  const char *match_serial_dfu = NULL;

  // This provides dfu_malloc without the need to compile
  // dfu_file.c and its dependencies.
  void *dfu_malloc(size_t size) { return malloc(size); }

  // This is the global DFU tree instance used in dfu_util.
  struct dfu_if *dfu_root = NULL;
}

using json = nlohmann::json;
using namespace std;

std::string print_hex(int number) {
  char tmp[10];
  snprintf(tmp, 10, "0x%04x", number);
  std::string s(tmp);
  return s;
}

/* Walk the device tree and print out DFU devices */
json get_dfu_interfaces(void) {
  struct dfu_if *pdfu;

  json j;
  j["event"] = "list";

  int index = 0;

  for (pdfu = dfu_root; pdfu != NULL; pdfu = pdfu->next) {

	if (!(pdfu->flags & DFU_IFF_DFU)) {
		// decide what to do with DFU runtime objects
		// for the time being, ignore them
		continue;
	}

	for (int i = 0; i < index; i++) {
		// allow only one object
		if (j["ports"][i]["address"] == get_path(pdfu->dev)) {
			index = i;
			break;
		}
	}

    j["ports"][index]["address"] = get_path(pdfu->dev);
    j["ports"][index]["label"] = pdfu->alt_name;
    j["ports"][index]["protocol"] = "dfu";
    j["ports"][index]["protocolLabel"] =
        pdfu->flags & DFU_IFF_DFU ? "DFU" : "Runtime";
    j["ports"][index]["properties"] = {{"vid", print_hex(pdfu->vendor)},
                                       {"pid", print_hex(pdfu->product)},
                                       {"serialNumber", pdfu->serial_name},
                                       {"name", pdfu->alt_name}};
    index++;
  }
  return j;
}

bool getline_async(std::istream &is, std::string &str, char delim = '\n') {

  static std::string lineSoFar;
  char inChar;
  int charsRead = 0;
  bool lineRead = false;
  str = "";

  do {
    charsRead = is.readsome(&inChar, 255);
    if (charsRead == 1) {
      // if the delimiter is read then return the string so far
      if (inChar == delim) {
        str = lineSoFar;
        lineSoFar = "";
        lineRead = true;
      } else { // otherwise add it to the string so far
        lineSoFar.append(charsRead, inChar);
      }
    }
  } while (charsRead != 0 && !lineRead);

  return lineRead;
}

void print_list() {
  auto j = get_dfu_interfaces();
  std::cout << j.dump(2) << std::endl;
}




json previous_list;
void print_event() {
  auto j = get_dfu_interfaces();
  auto diff = json::diff(previous_list, j);

  json diff_discovery;
  //diff_discovery["eventType"] = "remove";

  if (diff.size() > 0) {
    for (auto& new_port : j["ports"].items()) {
      bool found = false;
      for (auto& old_port : previous_list["ports"].items()) {
        if (new_port.value()["address"] == old_port.value()["address"]) {
          found = true;
          break;
        }
      }
      if (!found) {
        diff_discovery["eventType"] = "add";
        diff_discovery["port"] = new_port.value();
        std::cout << diff_discovery.dump(2) << std::endl;
      }
    }

    for (auto& new_port : previous_list["ports"].items()) {
      bool found = false;
      for (auto& old_port : j["ports"].items()) {
        if (new_port.value()["address"] == old_port.value()["address"]) {
          found = true;
          break;
        }
      }
      if (!found) {
        diff_discovery["eventType"] = "remove";
        diff_discovery["port"] = new_port.value();
        std::cout << diff_discovery.dump(2) << std::endl;
      }
    }
    //std::cout << diff.dump(2) << std::endl;
  }
  previous_list = j;
}

enum states { IDLE, START, START_SYNC, STOP, QUIT };

int main(int argc, char **argv) {
  libusb_context *ctx;
  int ret;
  int fd;

  /* make sure all prints are flushed */
  setvbuf(stdout, NULL, _IONBF, 0);

  /* Mute stderr */
  //freopen("/dev/null", "w", stderr);

  // See
  // https://arduino.github.io/arduino-cli/0.30/pluggable-discovery-specification/

  // wait for HELLO
  for (std::string line; std::getline(std::cin, line);) {
    if (line.find("HELLO") != std::string::npos) {
      break;
    }
  }

  // Set STDIN as nonblocking
  // int flags = fcntl(0, F_GETFL, 0);
  // fcntl(0, F_SETFL, flags | O_NONBLOCK);
  json j;

  j["eventType"] = "hello";
  j["protocolVersion"] = 1;
  j["message"] = "OK";

  std::cout << j.dump(2) << std::endl;

  std::atomic<int> state = { IDLE };

  while (1) {

    std::string line;
    std::getline(std::cin, line);

    if (line.find("START_SYNC") != std::string::npos) {
      if (state == IDLE) {
        ret = libusb_init(&ctx);
        j["eventType"] = "start_sync";
        if (ret != 0) {
          j["message"] = "Permission error";
          j["error"] = true;
          std::cout << j.dump(2) << std::endl;
          continue;
        }
        std::thread([&] {
          j["message"] = "OK";
          std::cout << j.dump(2) << std::endl;
          while (state != QUIT) {
            if (state == START_SYNC) {
              probe_devices(ctx);
              print_event();
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
          }
        }).detach();
      }
      state = START_SYNC;
    } else if (line.find("START") != std::string::npos) {
      state = START;
      j["eventType"] = "start";
      ret = libusb_init(&ctx);
      if (ret == 0) {
        j["message"] = "OK";
      } else {
        j["error"] = true;
        j["message"] = "Permission error";
      }
      std::cout << j.dump(2) << std::endl;
    } else if (line.find("STOP") != std::string::npos) {
      j["eventType"] = "stop";
      j["message"] = "OK";
      std::cout << j.dump(2) << std::endl;
      if (state != START_SYNC) {
        libusb_exit(ctx);
      }
      state = STOP;
    } else if (line.find("QUIT") != std::string::npos) {
      state = QUIT;
      j["eventType"] = "quit";
      j["message"] = "OK";
      std::cout << j.dump(2) << std::endl;
      exit(0);
    } else if (line.find("LIST") != std::string::npos) {
      probe_devices(ctx);
      print_list();
    }
  }
}
