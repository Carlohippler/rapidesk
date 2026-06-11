// src/network/media_channel.hpp
#pragma once
#include <array>
#include <boost/lockfree/stack.hpp>

struct MediaPacket {
    uint16_t sequence_number;
    uint16_t stream_id;
    uint32_t timestamp;
    uint32_t frame_id;
    uint8_t  flags;
    uint16_t fragment_index;
    uint8_t  reserved;
    std::array<uint8_t, 1400> payload;
};

class PacketPool {
public:
    static constexpr size_t POOL_SIZE = 256;
    PacketPool();
    MediaPacket* acquire() noexcept;
    void release(MediaPacket* p) noexcept;
private:
    std::array<MediaPacket, POOL_SIZE> pool_;
    boost::lockfree::stack<MediaPacket*> free_list_;
};