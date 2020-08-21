/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "protobag/archive/LibArchiveArchive.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <archive.h>
#include <archive_entry.h>
#include <fmt/format.h>

#include "protobag/Utils/Tempfile.hpp"

namespace protobag {
namespace archive {

namespace fs = std::filesystem;

bool LibArchiveArchive::IsSupported(const std::string &format) {
  return 
    format == "zip" || format == "tar";
}




// ============================================================================
// Impl Reader and Writer
// Based on:
//   https://github.com/libarchive/libarchive/wiki/Examples
//   moor: https://github.com/castedmo/moor

class LibArchiveArchive::ImplBase {
public:
  virtual ~ImplBase() {
    if (_archive) {
      if (_is_reading) {
        archive_read_close(_archive);
        archive_read_free(_archive);
      } else {
        archive_write_close(_archive);
        archive_write_free(_archive);
      }
    }
    _archive = nullptr;
  }
protected:
  struct archive* _archive = nullptr;
  bool _is_reading = true;

  void CheckOrThrow(int retcode) {
    auto err = CheckOrError(retcode);
    if (!err.empty()) {
      throw std::runtime_error(err);
    }
  }

  std::string CheckOrError(int retcode) {
    if (!_archive) {
      return "protobag: null archive";
    }
    if (retcode != ARCHIVE_OK && retcode != ARCHIVE_WARN) {
      auto e = archive_error_string(_archive);
      if (e == nullptr) {
        return fmt::format("LibArchive: No error, but got retcode {}", retcode);
      } else {
        return e;
      }
    }
    return "";
  }
};


class Reader : public LibArchiveArchive::ImplBase {
public:

  OkOrErr Open(Archive::Spec s) {
    if (_archive) {
      return {.error = "Programming error: archive already open"};
    }

    _is_reading = true;

    try {
    
      _archive = archive_read_new();
      CheckOrThrow(archive_read_support_filter_all(_archive));
      CheckOrThrow(archive_read_support_format_all(_archive));

      CheckOrThrow(
          archive_read_open_filename(
            _archive, s.path.c_str(), 10240));
      
    } catch (std::exception &e) {
      return OkOrErr::Err(
        fmt::format("Error while trying to open for reading: {}", e.what()));
    }

    return kOK;
  }

  std::vector<std::string> GetNamelist() {
    if (!_archive) { return {}; }

    std::vector<std::string> namelist;
    archive_entry *entry = nullptr;
    while (archive_read_next_header(_archive, &entry) == ARCHIVE_OK) {
      // Only record files, not directories
      if (archive_entry_filetype(entry) == AE_IFREG) {
        namelist.push_back(archive_entry_pathname_utf8(entry));
      }

      auto ret = archive_read_data_skip(_archive);
        // NB: libarchive will generally use a seek to skip if possible,
        // but worst case might actually read & throw away data.  FMI see
        // __archive_read_register_format() and/or
        // archive_read_extract_set_skip_file()

      std::string error = CheckOrError(ret);
      if (!error.empty()) {
        return {};
      }
    }

    return namelist;
  }
  
  Archive::ReadStatus ReadAsStr(const std::string &entryname) {
    if (!_archive) {
      return Archive::ReadStatus::Err("Archive not open for reading");
    }
      
    // A linear find is the best we can do with libarchive, but libarchive will
    // generally use seeks to skip entries if possible.  FMI see
    // __archive_read_register_format() and/or
    // archive_read_extract_set_skip_file()
    // https://github.com/libarchive/libarchive/wiki/Examples#a-note-about-the-skip-callback
    archive_entry *entry = nullptr;
    Archive::ReadStatus result = Archive::ReadStatus::EntryNotFound();
    while (archive_read_next_header(_archive, &entry) == ARCHIVE_OK) {
      std::string cur = archive_entry_pathname_utf8(entry);
      if (cur == entryname) {
        result = ReadEntry(entry);
        break;
      } else {
        std::string maybe_err = CheckOrError(archive_read_data_skip(_archive));
        if (!maybe_err.empty()) {
          result = Archive::ReadStatus::Err(maybe_err);
        }
      }
    }

    return result;
  }

  OkOrErr StreamingUnpackEntryTo(
            const std::string &entryname,
            const std::string &dest_dir) {

    if (!_archive) {
      return OkOrErr::Err("Archive not open for reading");
    }

    archive_entry *entry = nullptr;
    OkOrErr result =
      OkOrErr::Err(fmt::format("Entry {} not found", entryname));
    while (archive_read_next_header(_archive, &entry) == ARCHIVE_OK) {
      std::string cur = archive_entry_pathname_utf8(entry);
      if (cur == entryname) {
        
        // Based upon https://github.com/libarchive/libarchive/blob/c400064a1c63d122340d09d8ce3f671d4cf24b6e/examples/untar.c#L139
        struct archive *ext = archive_write_disk_new();
	      archive_write_disk_set_options(
          ext,
          (ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
            ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS));
        archive_write_disk_set_standard_lookup(ext);

        // Set extraction destination https://stackoverflow.com/a/25991813
        const char* entrypath = archive_entry_pathname(entry);
        const std::string output_path = fs::path(dest_dir) / entrypath;
        archive_entry_set_pathname(entry, output_path.c_str());

        int r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
          return OkOrErr::Err(
            fmt::format(
              "Libarchive failed to write file header for {}. Error: {}",
              entryname, 
              archive_error_string(ext)));
        } else {

          auto status = StreamingCopyData(_archive, ext);
          if (!status.IsOk()) {
            return OkOrErr::Err(
              fmt::format(
                "Libarchive failed while trying to write to {}: {}",
                entryname,
                status.error));
          } else {
            r = archive_write_finish_entry(ext);
            if (r != ARCHIVE_OK) {
              return OkOrErr::Err(
                fmt::format(
                  "Libarchive failed to finish extracting {}. Error: {}",
                  entryname, 
                  archive_error_string(ext)));
            }
          }

        }
        
        result = kOK;
        break;
      } else {
        std::string maybe_err = CheckOrError(archive_read_data_skip(_archive));
        if (!maybe_err.empty()) {
          result = OkOrErr::Err(maybe_err);
        }
      }
    }

    return result;
  }

protected:
  Archive::ReadStatus ReadEntry(archive_entry *entry) {
    if (!_archive) {
      return Archive::ReadStatus::Err("ReadEntry: archive not open");
    }
    if (!entry) {
      return Archive::ReadStatus::Err("ReadEntry: entry invalid");
    }

    std::string out;
    la_int64_t entry_size = archive_entry_size(entry);
    out.resize(entry_size);
    
    // libarchive only supports streaming out chunks
    {
      la_ssize_t ret = ARCHIVE_OK;
      la_int64_t pos = 0;
      while (pos < entry_size) {
        ret = archive_read_data(
                    _archive,
                    &out[pos],
                    entry_size - pos);
        if (ret < 0) {
          std::string error = CheckOrError(ret);
          if (!error.empty()) {
            return Archive::ReadStatus::Err(error);
          }
        }

        pos += ret;
      }
    }

    return Archive::ReadStatus::OK(std::move(out));
  }

  OkOrErr StreamingCopyData(struct archive *ar, struct archive *aw) {
	  int r;
	  const void *buff;
	  size_t size;
	  int64_t offset;

    for (;;) {
      r = archive_read_data_block(ar, &buff, &size, &offset);
      if (r == ARCHIVE_EOF) {
        return kOK;
      }
      if (r != ARCHIVE_OK) {
        return OkOrErr::Err(
          fmt::format("Read error: {}", archive_error_string(ar)));
      }
      r = archive_write_data_block(aw, buff, size, offset);
      if (r != ARCHIVE_OK) {
        return OkOrErr::Err(
          fmt::format("Write error: archive_write_data_block {}",
          archive_error_string(aw)));
      }
    }

    return kOK;
  }
};


class Writer : public LibArchiveArchive::ImplBase {
public:

  OkOrErr Open(Archive::Spec s) {
    if (_archive) {
      return {.error = "Programming error: archive already open"};
    }

    _is_reading = false;
    try {

      _archive = archive_write_new();
      if (s.format == "tar") {
        CheckOrThrow(archive_write_set_format_pax_restricted(_archive));
      } else if (s.format == "zip") {
        CheckOrThrow(archive_write_set_format_zip(_archive));
      } else {
        return OkOrErr::Err(
          fmt::format("LibArchiveArchive format not supported: {}", s.format));
      }

      CheckOrThrow(archive_write_open_filename(_archive, s.path.c_str()));

    } catch (std::exception &e) {
      return OkOrErr::Err(
        fmt::format("Error while trying to open for writing: {}", e.what()));
    }

    return kOK;
  }

  OkOrErr Write(const std::string &entryname, const std::string &data) {
    if (!_archive) { return OkOrErr::Err("ReadEntry: archive not open"); }

    OkOrErr result = OkOrErr::Err("unknown failure");
    archive_entry *entry = nullptr;
    try {
      entry = archive_entry_new();
      entry = archive_entry_clear(entry);
      archive_entry_set_pathname(entry, entryname.c_str());
      archive_entry_set_size(entry, data.size());
      archive_entry_set_filetype(entry, AE_IFREG);
      archive_entry_set_perm(entry, 0644);
      CheckOrThrow(archive_write_header(_archive, entry));
    
      archive_write_data(_archive, data.data(), data.size());
      result = kOK;

    } catch (std::exception &e) {
      result = OkOrErr::Err(
        fmt::format("Failed to write {} : {}", entryname, e.what()));
    }

    if (entry) {
      archive_entry_free(entry);
    }

    return result;
  }
  
  OkOrErr WriteUsingBufferedReads(
              const std::string &entryname,
              const std::string &path) {
      
    if (!_archive) { return OkOrErr::Err("ReadEntry: archive not open"); }

    

    OkOrErr result = OkOrErr::Err("unknown failure");
    archive_entry *entry = nullptr;
    try {
      entry = archive_entry_new();
      entry = archive_entry_clear(entry);
      archive_entry_set_pathname(entry, entryname.c_str());
      archive_entry_set_filetype(entry, AE_IFREG);
      archive_entry_set_perm(entry, 0644);

      // NB: http://0x80.pl/notesen/2019-01-07-cpp-read-file.html
      // We use C++ Filesystem POSIX-backed API because it's the fastest
      std::FILE* f = std::fopen(path.c_str(), "r");
      if (!f) {
        return result = OkOrErr::Err(fmt::format("Failed to read {}", path));
      } else {
        // Based upon minitar: 
        // https://github.com/libarchive/libarchive/blob/c400064a1c63d122340d09d8ce3f671d4cf24b6e/examples/minitar/minitar.c#L309
        
        const auto f_size = fs::file_size(path);
        archive_entry_set_size(entry, f_size);
        CheckOrThrow(archive_write_header(_archive, entry));

        thread_local static char buff[16384];
        size_t len = std::fread(buff, 1, sizeof(buff), f);
        while (len > 0) {
          archive_write_data(_archive, buff, len);
          len = std::fread(buff, 1, sizeof(buff), f);
        }
        std::fclose(f);
        result = kOK;
      }
    } catch (std::exception &e) {
      result = OkOrErr::Err(
        fmt::format("Failed to write {} : {}", entryname, e.what()));
    }

    if (entry) {
      archive_entry_free(entry);
    }

    return result;
  }
};

Result<Archive::Ptr> LibArchiveArchive::Open(Archive::Spec s) {
  Archive::Spec final_spec = s;
  if (s.mode == "read" && !fs::is_regular_file(s.path)) {

    return {.error = fmt::format("Can't find archive to read {}", s.path)};
  
  } else if (s.mode == "write" && s.path == "<tempfile>") {

    auto maybe_path =
      CreateTempfile(/*suffix=*/"_LibArchiveArchive." + s.format);
    if (maybe_path.IsOk()) {
      final_spec.path = *maybe_path.value;
    } else {
      return {.error = maybe_path.error};
    }
    
  }

  LibArchiveArchive *lar = new LibArchiveArchive();
  Archive::Ptr p(lar);
  lar->_spec = final_spec;
  

  if (lar->_spec.mode == "write") {
    Writer *pw = new Writer();
    std::shared_ptr<ImplBase> pi(pw);
    auto &w = *pw;

    OkOrErr r = w.Open(lar->_spec);
    if (!r.IsOk()) { return {.error = r.error}; }
    lar->_impl = pi;
    
    // TODO: consider supporting append mode 
    // https://groups.google.com/forum/#!msg/libarchive-discuss/dEqPZbnimVM/97Gp1LgWf7UJ 

  }

  return {.value = p};
}

std::vector<std::string> LibArchiveArchive::GetNamelist() {
  Reader reader;
  OkOrErr r = reader.Open(GetSpec());
  if (!r.IsOk()) {
    return {};
  }

  return reader.GetNamelist();
}

Archive::ReadStatus LibArchiveArchive::ReadAsStr(const std::string &entryname) {
  Reader reader;
  OkOrErr r = reader.Open(GetSpec());
  if (!r.IsOk()) {
    return Archive::ReadStatus::Err(r.error);
  }

  return reader.ReadAsStr(entryname);
}

OkOrErr LibArchiveArchive::Write(
    const std::string &entryname, const std::string &data) {

  if (!_impl) {
    return {.error = "LibArchiveArchive programming error: no writer"};
  }
  auto writer = std::dynamic_pointer_cast<Writer>(_impl);
  if (!writer) { 
    return {.error = "Archive not set up for writing.  Did you Open()?"};
  }

  return writer->Write(entryname, data);
}


// ============================================================================
// Streaming Unpack / Add API

OkOrErr LibArchiveArchive::StreamingUnpackEntryTo(
    const std::string &entryname,
    const std::string &dest_dir) {

  Reader reader;
  OkOrErr r = reader.Open(GetSpec());
  if (!r.IsOk()) {
    return OkOrErr::Err(r.error);
  }

  return reader.StreamingUnpackEntryTo(entryname, dest_dir);
}

OkOrErr LibArchiveArchive::StreamingAddFile(
    const std::string &src_file,
    const std::string &entryname) {

  if (!_impl) {
    return {.error = "LibArchiveArchive programming error: no writer"};
  }
  auto writer = std::dynamic_pointer_cast<Writer>(_impl);
  if (!writer) { 
    return {.error = "Archive not set up for writing.  Did you Open()?"};
  }

  return writer->WriteUsingBufferedReads(entryname, src_file);
}


} /* namespace archive */
} /* namespace protobag */
