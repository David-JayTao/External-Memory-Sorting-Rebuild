#include "external/run_file.h"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace external {
namespace {
constexpr std::uint64_t header_bytes = 48;
constexpr std::uint64_t entry_bytes = 32;
constexpr std::uint64_t value_bytes = 4;
constexpr char magic[] = "EXRUN001";
constexpr std::uint64_t max_offset =
    static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max());

void write_uint(std::ostream& stream, std::uint64_t value, std::size_t bytes) {
    for (std::size_t index = 0; index < bytes; ++index) {
        stream.put(static_cast<char>(value & 255));
        value >>= 8;
    }
    if (!stream) throw std::runtime_error("File write failed");
}
std::uint64_t read_uint(std::istream& stream, std::size_t bytes) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < bytes; ++index) {
        const int byte = stream.get();
        if (byte == std::char_traits<char>::eof())
            throw std::runtime_error("Truncated file");
        value |= static_cast<std::uint64_t>(byte) << (index * 8);
    }
    return value;
}
void write_header(std::ostream& stream, const FileHeader& header, bool complete) {
    stream.seekp(0);
    stream.write(complete ? magic : "PENDING!", 8);
    write_uint(stream, header.max_runs, 8);
    write_uint(stream, header.run_count, 8);
    write_uint(stream, header.directory_offset, 8);
    write_uint(stream, header.data_start_offset, 8);
    write_uint(stream, header.data_end_offset, 8);
}
}

RunFile::RunFile(const std::filesystem::path& path) : stream_(path, std::ios::binary) {
    if (!stream_) throw std::runtime_error("Cannot open input file");
    std::array<char, 8> signature{};
    stream_.read(signature.data(), 8);
    if (!stream_ || !std::equal(signature.begin(), signature.end(), magic))
        throw std::runtime_error("Invalid or unfinished run file");
    header_.max_runs = read_uint(stream_, 8);
    header_.run_count = read_uint(stream_, 8);
    header_.directory_offset = read_uint(stream_, 8);
    header_.data_start_offset = read_uint(stream_, 8);
    header_.data_end_offset = read_uint(stream_, 8);
    const auto file_bytes = std::filesystem::file_size(path);
    if (header_.max_runs > (max_offset - header_bytes) / entry_bytes ||
        header_.run_count > header_.max_runs ||
        header_.directory_offset != header_bytes ||
        header_.data_start_offset != header_bytes + entry_bytes * header_.max_runs ||
        header_.data_end_offset != file_bytes ||
        header_.data_start_offset > file_bytes || file_bytes > max_offset)
        throw std::runtime_error("Invalid header");
    std::uint64_t expected_offset = header_.data_start_offset;
    for (std::uint64_t run_id = 0; run_id < header_.run_count; ++run_id) {
        RunInfo run;
        run.id = read_uint(stream_, 8);
        run.record_count = read_uint(stream_, 8);
        run.data_offset = read_uint(stream_, 8);
        run.data_bytes = read_uint(stream_, 8);
        if (run.id != run_id || run.data_offset != expected_offset ||
            run.record_count > (file_bytes - expected_offset) / value_bytes ||
            run.data_bytes != run.record_count * value_bytes)
            throw std::runtime_error("Invalid directory entry");
        expected_offset += run.data_bytes;
        runs_.push_back(run);
    }
    if (expected_offset != file_bytes) throw std::runtime_error("Unreferenced data");
}

std::size_t RunFile::read_block(std::size_t run_id, std::uint64_t record_offset,
                               Value* values, std::size_t capacity) {
    const auto& run = runs_.at(run_id);
    if (record_offset > run.record_count) throw std::out_of_range("Record offset");
    const auto count = static_cast<std::size_t>(
        std::min<std::uint64_t>(capacity, run.record_count - record_offset));
    if (count && !values) throw std::invalid_argument("Null input buffer");
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(run.data_offset + record_offset * value_bytes));
    for (std::size_t index = 0; index < count; ++index) {
        const auto bits = read_uint(stream_, 4);
        // 显式转换二进制补码，避免超范围无符号到有符号转换的实现差异。
        const std::int64_t signed_value = bits <= 0x7fffffffULL
            ? static_cast<std::int64_t>(bits) : static_cast<std::int64_t>(bits) - 0x100000000LL;
        values[index] = static_cast<Value>(signed_value);
    }
    return count;
}

RunWriter::RunWriter(const std::filesystem::path& path, std::uint64_t max_runs) {
    if (max_runs > (max_offset - header_bytes) / entry_bytes)
        throw std::invalid_argument("Directory too large");
    if (std::filesystem::exists(path)) throw std::runtime_error("Output already exists");
    header_.max_runs = max_runs;
    header_.data_start_offset = header_bytes + entry_bytes * max_runs;
    header_.data_end_offset = header_.data_start_offset;
    stream_.open(path, std::ios::binary | std::ios::trunc);
    if (!stream_) throw std::runtime_error("Cannot create output file");
    write_header(stream_, header_, false);
    // 预留目录区，不分配与其大小相同的内存。
    if (max_runs) {
        stream_.seekp(static_cast<std::streamoff>(header_.data_start_offset - 1));
        stream_.put(0);
        if (!stream_) throw std::runtime_error("Cannot reserve directory");
    }
}
void RunWriter::begin_run() {
    if (finished_ || run_open_ || header_.run_count == header_.max_runs)
        throw std::logic_error("Cannot begin run");
    current_run_ = {header_.run_count, 0, header_.data_end_offset, 0};
    stream_.seekp(static_cast<std::streamoff>(header_.data_end_offset));
    run_open_ = true;
}
void RunWriter::append_block(const Value* values, std::size_t count) {
    if (!run_open_ || finished_) throw std::logic_error("No writable run");
    if (count && !values) throw std::invalid_argument("Null output buffer");
    if (count > (max_offset - header_.data_end_offset) / value_bytes)
        throw std::overflow_error("File too large");
    for (std::size_t index = 0; index < count; ++index)
        write_uint(stream_, static_cast<std::uint32_t>(values[index]), 4);
    current_run_.record_count += count;
    current_run_.data_bytes += count * value_bytes;
    header_.data_end_offset += count * value_bytes;
}
void RunWriter::end_run() {
    if (!run_open_) throw std::logic_error("No open run");
    stream_.seekp(static_cast<std::streamoff>(header_bytes + header_.run_count * entry_bytes));
    write_uint(stream_, current_run_.id, 8);
    write_uint(stream_, current_run_.record_count, 8);
    write_uint(stream_, current_run_.data_offset, 8);
    write_uint(stream_, current_run_.data_bytes, 8);
    ++header_.run_count;
    run_open_ = false;
}
void RunWriter::finish() {
    if (finished_ || run_open_) throw std::logic_error("Cannot finish file");
    write_header(stream_, header_, true);
    stream_.flush();
    if (!stream_) throw std::runtime_error("Flush failed");
    stream_.close();
    if (stream_.fail()) throw std::runtime_error("Close failed");
    finished_ = true;
}
}
