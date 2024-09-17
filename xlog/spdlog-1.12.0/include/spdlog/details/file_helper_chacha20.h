// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <tuple>
#ifdef __cplusplus
extern "C" {
#endif
#include <mbedtls/chacha20.h>
#ifdef __cplusplus
}
#endif
#include <memory>

namespace spdlog {
namespace details {

// Helper class for file sinks.
// When failing to open a file, retry several times(5) with a delay interval(10 ms).
// Throw spdlog_ex exception on errors.


class SPDLOG_API file_helper_chacha20
{
public:
    explicit file_helper_chacha20(const file_event_handlers &event_handlers,
                                  const uint8_t* key, const uint8_t* nonce);

    file_helper_chacha20(const file_helper_chacha20 &) = delete;
    file_helper_chacha20 &operator=(const file_helper_chacha20 &) = delete;
    ~file_helper_chacha20();

    void open(const filename_t &fname, bool truncate = false);
    void reopen(bool truncate);
    void flush();
    void close();
    void write(const memory_buf_t &buf);
    size_t size() const;
    const filename_t &filename() const;

    //
    // return file path and its extension:
    //
    // "mylog.txt" => ("mylog", ".txt")
    // "mylog" => ("mylog", "")
    // "mylog." => ("mylog.", "")
    // "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
    //
    // the starting dot in filenames is ignored (hidden files):
    //
    // ".mylog" => (".mylog". "")
    // "my_folder/.mylog" => ("my_folder/.mylog", "")
    // "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")
    static std::tuple<filename_t, filename_t> split_by_extension(const filename_t &fname);

private:
    void init_chacha20();
    const int open_tries_ = 5;
    const unsigned int open_interval_ = 10;
    std::FILE *fd_{nullptr};
    filename_t filename_;
    file_event_handlers event_handlers_;
    //chacha20
    mbedtls_chacha20_context* chacha20_ctx_ = nullptr;
    std::unique_ptr<uint8_t> crypto_buffer_;
    uint8_t key_[32];
    uint8_t nonce_[12];
    std::fpos_t size_pos_{};//加密数据大小文件头偏移,文件关闭前需要在此偏移处写入加密数据大小
    uint32_t total_write_ = 0;//加密数据大小
};
} // namespace details
} // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
#    include "file_helper_chacha20-inl.h"
#endif
