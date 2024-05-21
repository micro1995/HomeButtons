#include "mdi_helper.h"

#include "download.h"
#include "github_raw_cert.h"

static constexpr char HOST[] = "raw.githubusercontent.com";

static constexpr char MDI_URL[] =
    "https://raw.githubusercontent.com/micro1995/MDI-BMP/main/";

static constexpr char TEST_URL[] =
    "https://raw.githubusercontent.com/micro1995/MDI-BMP/main/64x64/alien.bmp";

static constexpr char FOLDER[] = "/mdi";

bool MDIHelper::begin() {
  if (spiffs_mounted_) {
    return true;
  }
  if (!SPIFFS.begin()) {
    error("Failed to mount SPIFFS file system");
    return false;
  }

  spiffs_mounted_ = true;
  debug("Mounted SPIFFS file system");
  return true;
}

void MDIHelper::add_size(uint16_t size) {
  if (num_sizes_ >= MAX_NUM_SIZES) {
    error("Size %d not added, max number of sizes reached", size);
    return;
  }
  sizes_[num_sizes_++] = size;
}

void MDIHelper::end() {
  if (!spiffs_mounted_) {
    return;
  }
  SPIFFS.end();
  spiffs_mounted_ = false;
  debug("Unmounted SPIFFS file system");
}

StaticString<MAX_PATH_LEN> MDIHelper::_get_path(const char* name,
                                                uint16_t size) {
  return StaticString<MAX_PATH_LEN>("%s/%d/%s.bmp", FOLDER, size, name);
}

bool MDIHelper::check_connection() {
  return download::check_connection(HOST, TEST_URL,
                                    github_raw_cert::DigiCert_Global_Root_G2);
}

bool MDIHelper::download(const char* name, uint16_t size) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }

  auto path = _get_path(name, size);

  if (SPIFFS.exists(path.c_str())) {
    info("'%s' size %d already exists", name, size);
    return true;
  }

  debug("Downloading '%s' size %d to '%s'", name, size, path.c_str());

  File file = SPIFFS.open(path.c_str(), FILE_WRITE, true);
  if (!file) {
    error("Failed to open '%s' for writing", path.c_str());
    return false;
  }

  StaticString<256> url("%s%dx%d/%s.bmp", MDI_URL, size, size, name);
  bool ret = download::download_file_https(
      HOST, url.c_str(), file,
      github_raw_cert::DigiCert_Global_Root_G2);
  if (ret) {
    info("Downloaded '%s' size: %d", name, size);
    return true;
  } else {
    error("Failed to download '%s' size: %d", name, size);
    SPIFFS.remove(path.c_str());
    return false;
  }
}

bool MDIHelper::download(const char* name) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }

  for (uint8_t i = 0; i < num_sizes_; ++i) {
    if (!download(name, sizes_[i])) {
      return false;
    }
  }
  return true;
}

bool MDIHelper::exists(const char* name, uint16_t size) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  auto path = _get_path(name, size);
  return SPIFFS.exists(path.c_str());
}

bool MDIHelper::exists_all_sizes(const char* name) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  for (uint8_t i = 0; i < num_sizes_; ++i) {
    if (!exists(name, sizes_[i])) {
      return false;
    }
  }
  return true;
}

File MDIHelper::get_file(const char* name, uint16_t size) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return File();
  }

  if (!exists(name, size)) {
    error("'%s' size %d does not exist", name, size);
    return File();
  }

  auto path = _get_path(name, size);
  debug("Opening '%s'", path.c_str());
  return SPIFFS.open(path.c_str(), FILE_READ);
}

size_t MDIHelper::get_free_space() {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return 0;
  }
  size_t free = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  debug("Free space: %d", free);
  return free;
}

bool MDIHelper::make_space(size_t size) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  if (get_free_space() > size) {
    return true;
  }
  info("Freeing space...");
  File root = SPIFFS.open(FOLDER);
  if (!root) {
    error("Failed to open '%s'", FOLDER);
    return false;
  }
  if (!root.isDirectory()) {
    error("'%s' is not a directory", FOLDER);
    return false;
  }
  uint16_t count = 0;
  size_t size_before = SPIFFS.usedBytes();
  while (get_free_space() < size) {
    File file = root.openNextFile();
    if (!file) {
      error("Failed to open next file");
      return false;
    }
    size_t len = strlen(file.path());
    char path[len + 1];
    strncpy(path, file.path(), len + 1);
    file.close();
    SPIFFS.remove(path);
    count++;
    debug("Removed '%s'", path);
    delay(10);
  }
  info("Removed %d files, freed %d bytes", count,
       size_before - SPIFFS.usedBytes());
  return true;
}

bool MDIHelper::remove(const char* name, uint16_t size) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  auto path = _get_path(name, size);
  debug("Removing '%s'", path.c_str());
  return SPIFFS.remove(path.c_str());
}
