#include "XLog.h"
#include <numeric>
#include <QFile>
#include <spdlog/async.h>
#include "spdlog/async_logger.h"
#include "spdlog/sinks/daily_file_sink.h"

#ifdef _WIN32
#include "spdlog/sinks/windebug_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#endif

#ifdef __APPLE__
#include "spdlog/sinks/apple_unifiedlog_sink.h"
#endif

std::mutex XLogMgr::mutex_self_;
XLogMgr* XLogMgr::self_ = nullptr;

#define CRYPT_LOG//开启加密日志
#define ENABLE_DEBUGOUT//开启控制台输出
#define MAXLINSIZE (1024 * 2)
const int MAX_FILES = 30;
static uint8_t CHACHA_KEY[32] = {
    0x48, 0xC1, 0xFA, 0x03, 0x48, 0x8B, 0xC2, 0x48, 0xC1, 0xE8, 0x3F, 0x48, 0x03, 0xD0, 0x49, 0x8B,
    0x44, 0x24, 0x10, 0x48, 0xC1, 0xE8, 0x07, 0x48, 0x3B, 0xD0, 0x49, 0x0F, 0x44, 0xD8, 0x48, 0x89
};

namespace spdlog {
namespace details {
static uint32_t CRYPTO_BUFFER_SIZE = 4096;


FileHelperChacha::FileHelperChacha(const file_event_handlers &event_headles)
    : event_handlers_(event_headles),
    crypto_buffer_(new uint8_t[CRYPTO_BUFFER_SIZE])
{
    std::memcpy(key_,CHACHA_KEY,32);
    std::memcpy(nonce_,CHACHA_KEY,12);
}

FileHelperChacha::~FileHelperChacha()
{
    close();
}
void FileHelperChacha::init_chacha20()
{
    //前四个字节全为0分隔每次加密的数据,后四个字节表示每次总计加密的数据大小
    unsigned char data[8] = {0,0,0,0,0xFF,0xFF,0xFF,0xFF};
    if (std::fwrite(data, 1, 8, fd_) != 8) {
        throw_spdlog_ex("Failed writing to file " + os::filename_to_str(filename_));
    }
 /*不记录加密大小
    total_write_ = 0;
    std::fpos_t pos;
    if (std::fgetpos(fd_, &pos) != 0) {
        throw_spdlog_ex("Failed getpos " + os::filename_to_str(filename_));
    }
#ifdef __linux__
    size_pos_.__pos = pos.__pos - 4;
#else
    size_pos_ = pos - 4;
#endif
*/
    if (!chacha20_ctx_) {
        chacha20_ctx_ = new mbedtls_chacha20_context;
    } else {
        delete chacha20_ctx_;
        chacha20_ctx_ = new mbedtls_chacha20_context;
    }
    mbedtls_chacha20_init(chacha20_ctx_);
    mbedtls_chacha20_setkey(chacha20_ctx_,key_);
    mbedtls_chacha20_starts(chacha20_ctx_,nonce_,0);
}
void FileHelperChacha::open(const filename_t &fname, bool truncate)
{
    close();
    filename_ = fname;

    auto *mode = SPDLOG_FILENAME_T("ab");
    auto *trunc_mode = SPDLOG_FILENAME_T("wb");

    if (event_handlers_.before_open)
    {
        event_handlers_.before_open(filename_);
    }

    for (int tries = 0; tries < open_tries_; ++tries)
    {
        // create containing folder if not exists already.
        os::create_dir(os::dir_name(fname));
        if (truncate)
        {
            // Truncate by opening-and-closing a tmp file in "wb" mode, always
            // opening the actual log-we-write-to in "ab" mode, since that
            // interacts more politely with eternal processes that might
            // rotate/truncate the file underneath us.
            std::FILE *tmp;
            if (os::fopen_s(&tmp, fname, trunc_mode))
            {
                continue;
            }
            std::fclose(tmp);
        }
        if (!os::fopen_s(&fd_, fname, mode))
        {
            if (event_handlers_.after_open)
            {
                event_handlers_.after_open(filename_, fd_);
            }
            init_chacha20();
            return;
        }

        details::os::sleep_for_millis(open_interval_);
    }

    throw_spdlog_ex("Failed opening file " + os::filename_to_str(filename_) + " for writing", errno);
}

void FileHelperChacha::reopen(bool truncate)
{
    if (filename_.empty())
    {
        throw_spdlog_ex("Failed re opening file - was not opened before");
    }
    this->open(filename_, truncate);
}

void FileHelperChacha::flush()
{
    std::fflush(fd_);
}

void FileHelperChacha::close()
{
    if (chacha20_ctx_ != nullptr) {
        delete chacha20_ctx_;
        chacha20_ctx_ = nullptr;
    }
    if (fd_ != nullptr)
    {
        if (event_handlers_.before_close)
        {
            event_handlers_.before_close(filename_, fd_);
        }

        std::fclose(fd_);
        fd_ = nullptr;

        if (event_handlers_.after_close)
        {
            event_handlers_.after_close(filename_);
        }
    }
}

void FileHelperChacha::write(const memory_buf_t &buf)
{
    size_t msg_size = buf.size();
    auto data = buf.data();
    if (msg_size > CRYPTO_BUFFER_SIZE) {
        crypto_buffer_.reset(new uint8_t[msg_size]);
    }
    mbedtls_chacha20_update(chacha20_ctx_,msg_size,(const uint8_t*)data,crypto_buffer_.get());
    if (std::fwrite(crypto_buffer_.get(), 1, msg_size, fd_) != msg_size)
    {
        throw_spdlog_ex("Failed writing to file " + os::filename_to_str(filename_), errno);
    }
    //total_write_ += msg_size;
}

size_t FileHelperChacha::size() const
{
    if (fd_ == nullptr)
    {
        throw_spdlog_ex("Cannot use size() on closed file " + os::filename_to_str(filename_));
    }
    return os::filesize(fd_);
}

const filename_t &FileHelperChacha::filename() const
{
    return filename_;
}

std::tuple<filename_t, filename_t> FileHelperChacha::split_by_extension(const filename_t &fname)
{
    auto ext_index = fname.rfind('.');

    // no valid extension found - return whole path and empty string as
    // extension
    if (ext_index == filename_t::npos || ext_index == 0 || ext_index == fname.size() - 1)
    {
        return std::make_tuple(fname, filename_t());
    }

    // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
    auto folder_index = fname.find_last_of(details::os::folder_seps_filename);
    if (folder_index != filename_t::npos && folder_index >= ext_index - 1)
    {
        return std::make_tuple(fname, filename_t());
    }

    // finally - return a valid base and extension tuple
    return std::make_tuple(fname.substr(0, ext_index), fname.substr(ext_index));
}
}
namespace sinks {
using daily_file_chacha20_sink_mt = daily_file_sink<std::mutex,details::FileHelperChacha, daily_filename_format_calculator>;

}
}

/*
QByteArray qCleanupFuncinfo(QByteArray info)
{
    // Strip the function info down to the base function name
    // note that this throws away the template definitions,
    // the parameter types (overloads) and any const/volatile qualifiers.

    if (info.isEmpty())
        return info;

    int pos;

    // Skip trailing [with XXX] for templates (gcc), but make
    // sure to not affect Objective-C message names.
    pos = info.size() - 1;
    if (info.endsWith(']') && !(info.startsWith('+') || info.startsWith('-'))) {
        while (--pos) {
            if (info.at(pos) == '[')
                info.truncate(pos);
        }
    }

    // operator names with '(', ')', '<', '>' in it
    static const char operator_call[] = "operator()";
    static const char operator_lessThan[] = "operator<";
    static const char operator_greaterThan[] = "operator>";
    static const char operator_lessThanEqual[] = "operator<=";
    static const char operator_greaterThanEqual[] = "operator>=";

    // canonize operator names
    info.replace("operator ", "operator");

    // remove argument list
    forever {
        int parencount = 0;
        pos = info.lastIndexOf(')');
        if (pos == -1) {
            // Don't know how to parse this function name
            return info;
        }

        // find the beginning of the argument list
        --pos;
        ++parencount;
        while (pos && parencount) {
            if (info.at(pos) == ')')
                ++parencount;
            else if (info.at(pos) == '(')
                --parencount;
            --pos;
        }
        if (parencount != 0)
            return info;

        info.truncate(++pos);

        if (info.at(pos - 1) == ')') {
            if (info.indexOf(operator_call) == pos - (int)strlen(operator_call))
                break;

            // this function returns a pointer to a function
            // and we matched the arguments of the return type's parameter list
            // try again
            info.remove(0, info.indexOf('('));
            info.chop(1);
            continue;
        } else {
            break;
        }
    }

    // find the beginning of the function name
    int parencount = 0;
    int templatecount = 0;
    --pos;

    // make sure special characters in operator names are kept
    if (pos > -1) {
        switch (info.at(pos)) {
        case ')':
            if (info.indexOf(operator_call) == pos - (int)strlen(operator_call) + 1)
                pos -= 2;
            break;
        case '<':
            if (info.indexOf(operator_lessThan) == pos - (int)strlen(operator_lessThan) + 1)
                --pos;
            break;
        case '>':
            if (info.indexOf(operator_greaterThan) == pos - (int)strlen(operator_greaterThan) + 1)
                --pos;
            break;
        case '=': {
            int operatorLength = (int)strlen(operator_lessThanEqual);
            if (info.indexOf(operator_lessThanEqual) == pos - operatorLength + 1)
                pos -= 2;
            else if (info.indexOf(operator_greaterThanEqual) == pos - operatorLength + 1)
                pos -= 2;
            break;
        }
        default:
            break;
        }
    }

    while (pos > -1) {
        if (parencount < 0 || templatecount < 0)
            return info;

        char c = info.at(pos);
        if (c == ')')
            ++parencount;
        else if (c == '(')
            --parencount;
        else if (c == '>')
            ++templatecount;
        else if (c == '<')
            --templatecount;
        else if (c == ' ' && templatecount == 0 && parencount == 0)
            break;

        --pos;
    }
    info = info.mid(pos + 1);

    // remove trailing '*', '&' that are part of the return argument
    while ((info.at(0) == '*')
           || (info.at(0) == '&'))
        info = info.mid(1);

    // we have the full function name now.
    // clean up the templates
    while ((pos = info.lastIndexOf('>')) != -1) {
        if (!info.contains('<'))
            break;

        // find the matching close
        int end = pos;
        templatecount = 1;
        --pos;
        while (pos && templatecount) {
            char c = info.at(pos);
            if (c == '>')
                ++templatecount;
            else if (c == '<')
                --templatecount;
            --pos;
        }
        ++pos;
        info.remove(pos, end - pos + 1);
    }

    return info;
}
*/

XLogMgr::XLogMgr()
{
    //threadpool
    spdlog::init_thread_pool(128,1);
    //qInstallMessageHandler(XLogMgr::QtMsgOutput);
}

void XLogMgr::StopLog()
{
    //qInstallMessageHandler(nullptr);
}

XLogMgr::~XLogMgr()
{
    if (logger_) logger_->flush();
}

#if 0
void XLogMgr::QtMsgOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QSet<QByteArray> funcname_set;
    static std::mutex mutex_functionset_;


    QByteArray clean_func = qCleanupFuncinfo(context.function);

    mutex_functionset_.lock();
    // 由于 log函数中使用 spdlog::source_loc 结构体 并未开辟自己的内存而是使用传入的char*
    // 且使用的是多线程的方案 如果使用栈内存，那么有可能真正打印的时候 这部分数据就是脏数据
    // 这里使用 static QSet<QByteArray> 来保存数据以保证生命周期，解决函数名是乱码的问题
    QByteArray &const_func_array = clean_func;
    if (funcname_set.contains(clean_func)) {
        const_func_array = *(funcname_set.find(clean_func));
    } else {
        const_func_array = *(funcname_set.insert(clean_func));
    }
    mutex_functionset_.unlock();

    switch (type) {
    case QtDebugMsg:
        XLogMgr::get()->Log(context.file,context.line, const_func_array.constData(),spdlog::level::debug,msg);
        break;
    case QtInfoMsg:
        XLogMgr::get()->Log(context.file,context.line,const_func_array.constData(),spdlog::level::info,msg);
        break;
    case QtWarningMsg:
        XLogMgr::get()->Log(context.file,context.line,const_func_array.constData(),spdlog::level::warn,msg);
        break;
    case QtCriticalMsg:
        XLogMgr::get()->Log(context.file,context.line,const_func_array.constData(),spdlog::level::err,msg);
        break;
    case QtFatalMsg:
        XLogMgr::get()->Log(context.file,context.line,const_func_array.constData(),spdlog::level::err,msg);
        break;
    }
}
#endif

void XLogMgr::InitLog(const std::string& path, const std::string& filename_pre, const std::string& logger_name)
{
    const std::string strSpdName = logger_name.empty() ? "untitled" : logger_name;
    spdlog::level::level_enum level = spdlog::level::info;

    std::string log_path = path + "/" + filename_pre + ".log";

    try {

        //sinks
#if defined(CRYPT_LOG)
        //加密日志后缀为slog
        log_path = log_path.replace(log_path.end()-4,log_path.end(),".slog");
#ifdef _WIN32
        auto daily_file_sink = std::make_shared<spdlog::sinks::daily_file_chacha20_sink_mt>(log_path,
                                                                                            0,0,
                                                                                            false,MAX_FILES);
#else
        auto daily_file_sink = std::make_shared<spdlog::sinks::daily_file_chacha20_sink_mt>(log_path.toStdString(),
                                                                                            0,0,
                                                                                            false,MAX_FILES);
#endif
#else
        auto daily_file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(log_path,
                                                                                   0,0,
                                                                                   false,MAX_FILES);
#endif
        std::vector<spdlog::sink_ptr> sinks;
        sinks.emplace_back(daily_file_sink);

#if defined(ENABLE_DEBUGOUT)
    #if defined(_WIN32)
        auto debug_sink = std::make_shared<spdlog::sinks::windebug_sink_mt>();
        sinks.emplace_back(debug_sink);
    #else
        auto debug_sink = std::make_shared<spdlog::sinks::apple_unifiedlog_sink_mt>();
        sinks.emplace_back(debug_sink);
    #endif
#endif
        //logger
        logger_ = std::make_shared<spdlog::async_logger>(strSpdName,
                                                         sinks.begin(),sinks.end(),
                                                         spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
        logger_->set_pattern("[%Y%m%d %H:%M:%S.%e] [%n] [%l] [%s:%#-%!] [thread:%t] %v");
        logger_->set_level(level);
        logger_->flush_on(spdlog::level::trace);

        spdlog::register_logger(logger_);
    }  catch (const spdlog::spdlog_ex& ex) {
        OutputDebugStringA(ex.what());
        return;
    }

}

int Decrypt(const QString &log_file)
{
    QString out_file = log_file;
    out_file = out_file.replace(".slog",".log");
    if (out_file == log_file) return -1;

    QFile file_slog(log_file);
    QFile file_out(out_file);

    QByteArray slog;
    if (file_slog.open(QIODevice::ReadOnly)) {
        slog = file_slog.readAll();
        file_slog.close();
    } else {
        return -2;
    }

    if (!file_out.open(QIODevice::ReadWrite)) {
        return -3;
    }

    int8_t head_flag[8] = {0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF};
    QList<qint64> headpos_list;
    int index_find = 0;
    for (;;) {
        index_find = slog.indexOf(QByteArray::fromRawData((const char*)head_flag, 8),index_find);
        if (index_find == -1) {
            break;
        }

        index_find += 8;
        headpos_list.append(index_find);
    }
    //在尾部添加一个虚拟头部0123456789
    headpos_list.append(file_slog.size()+8);

    for (int i = 0;i < headpos_list.size()-1;++i) {
        int pos = headpos_list.at(i);
        uint8_t* data = (uint8_t*)(slog.data()+pos);
        uint32_t size = headpos_list.at(i+1) - pos - 8;

        mbedtls_chacha20_context ctx;
        mbedtls_chacha20_init(&ctx);
        mbedtls_chacha20_setkey(&ctx,CHACHA_KEY);
        mbedtls_chacha20_starts(&ctx,CHACHA_KEY,0);

        std::unique_ptr<uint8_t[]> out_data(new uint8_t[size]);
        mbedtls_chacha20_update(&ctx,size,data,out_data.get());

        file_out.write((const char*)out_data.get(),size);
    }
    file_out.close();

    return 0;
}



void XLogMgr::SetLevel(spdlog::level::level_enum l)
{
    // if (qEnvironmentVariable("SHMDEV") == "1") {
    //     l = spdlog::level::trace;
    // }
    if (logger_) logger_->set_level(l);
}
