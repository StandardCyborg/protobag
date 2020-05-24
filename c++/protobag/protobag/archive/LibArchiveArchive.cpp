#include "protobag/archive/LibArchiveArchive.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <archive.h>
#include <archive_entry.h>
#include <fmt/format.h>

                                                                          // #include <iostream>

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
                                          // std::cout << "entryname " << entryname << " cur " << cur << std::endl;
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
      int ret = ARCHIVE_OK;
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
        return OkOrErr::Err(fmt::format("LibArchiveArchive format not supported: {}", s.format));
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
};





// struct LibArchiveArchive::libarchive_data {
//   struct archive* archive = nullptr;
//   struct archive_entry* entry = nullptr;

//   bool is_reading = false;

//   void CheckOrThrow(int retcode) {
//     auto err = CheckOrError(retcode);
//     if (!err.empty()) {
//       throw std::runtime_error(err);
//     }
//   }

//   std::string CheckOrError(int retcode) {
//     if (!archive) {
//       return "protobag: null archive";
//     }
//     if (retcode != ARCHIVE_OK && retcode != ARCHIVE_WARN) {
//       auto e = archive_error_string(archive);
//       if (e == nullptr) {
//         return fmt::format("LibArchive: No error, but got retcode {}", retcode);
//       } else {
//         return e;
//       }
//     }
//     return "";
//   }

//   Result<std::string> ReadCurrentEntry() {
//     if (!archive) { return {.error = "ReadCurrentEntry: archive not open"}; }
//     if (!entry) { return {.error = "ReadCurrentEntry: entry invalid"}; }

//     std::string out;
//     size_t entry_size = archive_entry_size(entry);
//     out.resize(entry_size);
    
//     {
//       int ret = ARCHIVE_OK;
//       size_t pos = 0;
//       std::string error;
//       while (pos < entry_size) {
//         ret = archive_read_data(
//                     archive,
//                     &out[pos],
//                     entry_size - pos);
//         if (ret < 0) {
//           error = CheckOrError(ret);
//           if (!error.empty()) {
//             return {.error = error};
//           }
//         }

//         pos += ret;
//       }
//     }

//     return {.value = out};
//   }

//   ~libarchive_data() {
//     if (archive) {
//       if (is_reading) {
//         archive_read_close(archive);
//         archive_read_free(archive);
//       } else {
//         archive_write_close(archive);
//         archive_write_free(archive);
//       }
//     }
//     archive = nullptr;

//     if (entry) {
//       archive_entry_free(entry);
//     }
//     entry = nullptr;
//   }
// };


Result<Archive::Ptr> LibArchiveArchive::Open(Archive::Spec s) {
  Archive::Spec final_spec = s;
  if (s.mode == "read" && !fs::is_regular_file(s.path)) {

    return {.error = fmt::format("Can't find archive to read {}", s.path)};
  
  } else if (s.mode == "write" && s.path == "<tempfile>") {

    auto maybe_path = CreateTempfile(/*suffix=*/"_LibArchiveArchive"); // ~~~~~~~~~~~~~~~ suffix from encoding ~~~~~~~~~~~~~~~~~~~~~
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
    
    // append mode https://groups.google.com/forum/#!msg/libarchive-discuss/dEqPZbnimVM/97Gp1LgWf7UJ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  }

  return {.value = p};
}

  // LibArchiveArchive *lar = new LibArchiveArchive();
  // Archive::Ptr p(lar);
  // lar->_spec = final_spec;
  // lar->_lib.reset(new LibArchiveArchive::libarchive_data());
  // auto &ar = *(lar->_lib);

  // if (s.mode == "write") {
    
  //   xxx
  //   // append mode https://groups.google.com/forum/#!msg/libarchive-discuss/dEqPZbnimVM/97Gp1LgWf7UJ

  // } else if (s.mode == "read") {
    
  //   xxxx
  
  // } else {
  //   return {.error=fmt::format("Unsupported mode {}", s.mode)};
  // }

  // return {.value = p};
// }


// std::shared_ptr<LibArchiveArchive::libarchive_data> LibArchiveArchive::GetReadable() {
//   if (!_lib) { return nullptr; }
  
//   std::shared_ptr<libarchive_data> lib = _lib;
//   if (!lib->is_reading) {
//     // If we're writing, re-open the current archive for reading
//     auto r_spec = GetSpec();
//     r_spec.mode = "read";
//     auto maybe_arch = Open(r_spec);
//     if (!maybe_arch.IsOk()) {
//       return {};
//     }
//     auto darch = std::dynamic_pointer_cast<LibArchiveArchive>(*maybe_arch.value);
//     if (!darch) { return nullptr; }
//     lib = darch->_lib;
//   }
//   return lib;
// }

// std::vector<std::string> LibArchiveArchive::GetNamelist() {
//   auto lib = GetReadable();
//   if (!lib) { return {}; }
  
//   // Iterate over all entries and collect names
//   auto &ar = *lib;
//   std::vector<std::string> entries;
//   // archive_entry_clear(ar.entry);
//   archive_entry *entry; // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ wtf
//   // while (archive_read_next_header2(ar.archive, ar.entry) == ARCHIVE_OK) {
//   while (archive_read_next_header(ar.archive, &entry) == ARCHIVE_OK) {
//     // entries.push_back(archive_entry_pathname_utf8(ar.entry));

//     // Ignore non-files
//     if (archive_entry_filetype(entry) == AE_IFREG) {
//       entries.push_back(archive_entry_pathname_utf8(entry));
//     }

//     auto ret = archive_read_data_skip(ar.archive);
//       // NB: libarchive will generally use a seek to skip if possible,
//       // but worst case might actually read & throw away data.  FMI see
//       // __archive_read_register_format() and/or
//       // archive_read_extract_set_skip_file()

//     ar.CheckOrThrow(ret); // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~removeme?
//   }

//   return entries;
// }

std::vector<std::string> LibArchiveArchive::GetNamelist() {
  Reader reader;
  OkOrErr r = reader.Open(GetSpec());
  if (!r.IsOk()) {
    return {};
  }

  return reader.GetNamelist();
}

// Result<std::string> 
//   LibArchiveArchive::ReadAsStr(const std::string &entryname) {
//                                                             std::cout << "ReadAsStr " << std::endl;

//   if (!_lib) { return {.error = "LibArchive not initialized"}; }
//   if (!_lib->archive) { return {.error = "LibArchive archive not opened"}; }

//   auto lib = GetReadable();
//   if (!lib) { return {.error = "Can't get readable handle to archive"}; }

//   // A linear find is the best we can do with libarchive, but libarchive will
//   // generally use seeks to skip entries if possible.  FMI see
//   // __archive_read_register_format() and/or
//   // archive_read_extract_set_skip_file()
//   // https://github.com/libarchive/libarchive/wiki/Examples#a-note-about-the-skip-callback
//   auto &ar = *lib;
//   // archive_entry_clear(ar.entry);
//   archive_entry *entry = nullptr; // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ wtf
//   Result<std::string> result{.error = fmt::format("Entry {} not found", entryname)};
//   while (archive_read_next_header(ar.archive, &entry) == ARCHIVE_OK) {
//     std::string e = archive_entry_pathname_utf8(entry);
//                                         std::cout << "entryname " << entryname << " e " << e << std::endl;
//     if (e == entryname) {
//       ar.entry = entry;
//       result = ar.ReadCurrentEntry();
//     } else {
//       std::string maybe_err = 
//         ar.CheckOrError(archive_read_data_skip(ar.archive));
//       if (!maybe_err.empty()) {
//         result = {.error = maybe_err};
//       }
//     }
//   }

//   return result;
// }

Archive::ReadStatus LibArchiveArchive::ReadAsStr(const std::string &entryname) {
  Reader reader;
  OkOrErr r = reader.Open(GetSpec());
  if (!r.IsOk()) {
    return Archive::ReadStatus::Err(r.error);
  }

  return reader.ReadAsStr(entryname);
}

// Result<bool> LibArchiveArchive::Write(
//     const std::string &entryname, const std::string &data) {

//   if (!_lib) { return {.error = "LibArchive not initialized"}; }
//   auto &ar = *_lib;
//   if (!ar.archive) { return {.error = "LibArchive archive not opened" }; }
  
//   try {
//     if (!ar.entry) { ar.entry = archive_entry_new(); }
//     archive_entry_clear(ar.entry);
//     archive_entry_set_pathname(ar.entry, entryname.c_str());
//     archive_entry_set_size(ar.entry, data.size());
//     archive_entry_set_filetype(ar.entry, AE_IFREG);
//     archive_entry_set_perm(ar.entry, 0644);
//     ar.CheckOrThrow(archive_write_header(ar.archive, ar.entry));
  
//     archive_write_data(ar.archive, data.data(), data.size());61

//   } catch (std::exception &e) {
//     return {.error = fmt::format("Failed to write {} : {}", entryname, e.what())};
//   }

//   return {.value = true};
// }

OkOrErr LibArchiveArchive::Write(
    const std::string &entryname, const std::string &data) {

  if (!_impl) { return {.error = "programming error: no writer"}; }
  auto writer = std::dynamic_pointer_cast<Writer>(_impl);
  if (!writer) { return {.error = "Archive not set up for writing.  Did you Open()?"}; }

  return writer->Write(entryname, data);
}

} /* namespace archive */
} /* namespace protobag */
