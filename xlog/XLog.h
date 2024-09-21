#pragma once

#include <chrono>
#include <string>
#include <spdlog/spdlog.h>
extern "C" {
#include <mbedtls/chacha20.h>
}

namespace spdlog {
namespace details {
class FileHelperChacha
{
public:
    explicit FileHelperChacha(const file_event_handlers &event_handlers);

    FileHelperChacha(const FileHelperChacha &) = delete;
    FileHelperChacha &operator=(const FileHelperChacha &) = delete;
    ~FileHelperChacha();

    void open(const filename_t &fname, bool truncate = false);
    void reopen(bool truncate);
    void flush();
    void close();
    void write(const memory_buf_t &buf);
    size_t size() const;
    const filename_t &filename() const;
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
    //std::fpos_t size_pos_{};//加密数据大小文件头偏移,文件关闭前需要在此偏移处写入加密数据大小
    //uint32_t total_write_ = 0;//加密数据大小
};
}//details

}//spdlog

// namespace fmt {
// template <> struct formatter<QString> {
//     char presentation = 'q';
//     constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
//       return ctx.begin();
//     }
//     template <typename FormatContext>
//     auto format(const QString& p, FormatContext& ctx) const -> decltype(ctx.out()) {
//         return fmt::format_to(ctx.out(), "{}", p.toUtf8().toStdString());
//     }
// };

// }

class XLogMgr
{
public:
    XLogMgr();
    ~XLogMgr();
    static XLogMgr* get()
    {
        if (self_) return self_;
        mutex_self_.lock();
        if (self_ == nullptr) self_ = new XLogMgr();
        mutex_self_.unlock();
        return self_;
    }
    static void release()
    {
        delete self_;
        self_ = nullptr;
    }
    void InitLog(const std::string &path, const std::string &filename_pre, const std::string &logger_name = std::string());

    template<typename... Args>
    void Log(const char* file,int line,const char* func,
             spdlog::level::level_enum lvl,
             spdlog::format_string_t<Args...> fmt, Args&&...args)
    {
        if (logger_) {
            logger_->log(spdlog::source_loc{file?file:"unknown",line,func?func:"unknown"},
                         lvl,
                         fmt,
                         std::forward<Args>(args)...);
        }
    }
    // void Log(const char* file,int line,const char* func, spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt)
    // {
    //     if (logger_) logger_->log(spdlog::source_loc{file?file:"unknown",line,func?func:"unknown"},lvl,fmt);
    // }

    // void Log(spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt)
    // {
    //     if (logger_) logger_->log(lvl,fmt);
    // }
    // void PerformanceBegin(const QString& key);
    // void PerformanceEnd(const QString& key);
    void SetLevel(spdlog::level::level_enum l);
    void StopLog();
    // static void QtMsgOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
private:
    static XLogMgr* self_;
    static std::mutex mutex_self_;
    std::shared_ptr<spdlog::logger> logger_;
};

#define SDEBUG(...)   XLogMgr::get()->Log(__FILE__, __LINE__, SPDLOG_FUNCTION,spdlog::level::debug,__VA_ARGS__)
#define SINFO(...)    XLogMgr::get()->Log(__FILE__, __LINE__, SPDLOG_FUNCTION,spdlog::level::info,__VA_ARGS__)
#define SWARN(...)   XLogMgr::get()->Log(__FILE__, __LINE__, SPDLOG_FUNCTION,spdlog::level::warn,__VA_ARGS__)
#define SERROR(...)   XLogMgr::get()->Log(__FILE__, __LINE__, SPDLOG_FUNCTION,spdlog::level::err,__VA_ARGS__)

