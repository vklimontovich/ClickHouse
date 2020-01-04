#include <IO/CompressionMethod.h>

#include <IO/ReadBuffer.h>
#include <IO/WriteBuffer.h>
#include <IO/ZlibInflatingReadBuffer.h>
#include <IO/ZlibDeflatingWriteBuffer.h>
#include <IO/BrotliReadBuffer.h>
#include <IO/BrotliWriteBuffer.h>

#include <Common/config.h>


namespace DB
{

namespace ErrorCodes
{
    extern const int NOT_IMPLEMENTED;
}


std::string toContentEncodingName(CompressionMethod method)
{
    switch (method)
    {
        case CompressionMethod::Gzip:   return "gzip";
        case CompressionMethod::Zlib:   return "deflate";
        case CompressionMethod::Brotli: return "br";
        case CompressionMethod::None:   return "";
    }
    __builtin_unreachable();
}


CompressionMethod chooseCompressionMethod(const std::string & path, const std::string & hint)
{
    std::string file_extension;
    if (hint.empty() || hint == "auto")
    {
        auto pos = path.find_last_of('.');
        if (pos != std::string::npos)
            file_extension = path.substr(pos + 1, std::string::npos);
    }

    const std::string * method_str = file_extension.empty() ? &hint : &file_extension;

    if (*method_str == "gzip" || *method_str == "gz")
        return CompressionMethod::Gzip;
    if (*method_str == "deflate")
        return CompressionMethod::Zlib;
    if (*method_str == "brotli" || *method_str == "br")
        return CompressionMethod::Brotli;
    if (*method_str == "none" || *method_str == "auto")
        return CompressionMethod::None;
    if (!file_extension.empty())
        return CompressionMethod::None; /// Unrecognized file extension.

    throw Exception("Unknown compression method " + hint + ". Only 'auto', 'none', 'gzip', 'br' are supported as compression methods",
        ErrorCodes::NOT_IMPLEMENTED);
}


std::unique_ptr<ReadBuffer> wrapReadBufferWithCompressionMethod(std::unique_ptr<ReadBuffer> nested, CompressionMethod method)
{
    if (method == CompressionMethod::Gzip || method == CompressionMethod::Zlib)
        return std::make_unique<ZlibInflatingReadBuffer>(std::move(nested), method);
#if USE_BROTLI
    if (method == CompressionMethod::Brotli)
        return std::make_unique<BrotliReadBuffer>(std::move(nested));
#endif

    if (method == CompressionMethod::None)
        return nested;

    throw Exception("Unsupported compression method", ErrorCodes::NOT_IMPLEMENTED);
}


std::unique_ptr<WriteBuffer> wrapWriteBufferWithCompressionMethod(std::unique_ptr<WriteBuffer> nested, CompressionMethod method, int level)
{
    if (method == DB::CompressionMethod::Gzip || method == CompressionMethod::Zlib)
        return std::make_unique<ZlibDeflatingWriteBuffer>(std::move(nested), method, level);

#if USE_BROTLI
    if (method == DB::CompressionMethod::Brotli)
        return std::make_unique<BrotliWriteBuffer>(std::move(nested), level);
#endif

    if (method == CompressionMethod::None)
        return nested;

    throw Exception("Unsupported compression method", ErrorCodes::NOT_IMPLEMENTED);
}

}
