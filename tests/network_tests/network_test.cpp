#include <gtest/gtest.h>
#include "network/media_channel.hpp"
#include "network/bitrate_controller.hpp"
#include <thread>
#include <chrono>

using namespace rapiddesk::network;

// --- Packet Pool Tests ---

TEST(PacketPoolTest, AcquireReleaseNoAllocation) {
    PacketPool pool;

    auto* p1 = pool.acquire();
    ASSERT_NE(p1, nullptr);

    // Fill with pattern
    std::memset(p1->data, 0xAB, 100);
    p1->size = 100;

    pool.release(p1);

    // Reacquire — should be same pointer (recycled)
    auto* p2 = pool.acquire();
    EXPECT_EQ(p1, p2);
}

TEST(PacketPoolTest, PoolExhaustion) {
    PacketPool pool;
    std::vector<Packet*> packets;

    // Exhaust pool
    for (size_t i = 0; i < PacketPool::POOL_SIZE + 10; ++i) {
        auto* p = pool.acquire();
        if (p) {
            packets.push_back(p);
        }
        else {
            EXPECT_GE(i, PacketPool::POOL_SIZE);
            break;
        }
    }

    for (auto* p : packets) {
        pool.release(p);
    }
}

TEST(PacketPoolTest, ThreadSafety) {
    PacketPool pool;
    std::atomic<int> success_count{ 0 };

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 1000; ++i) {
                auto* p = pool.acquire();
                if (p) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    pool.release(p);
                    success_count++;
                }
            }
            });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(success_count.load(), 4000);
}

// --- Bitrate Controller Tests ---

class BitrateControllerTest : public ::testing::Test {
protected:
    BitrateController controller_;
};

TEST_F(BitrateControllerTest, InitialBitrate) {
    EXPECT_EQ(controller_.current_bitrate(), 2'000'000);
}

TEST_F(BitrateControllerTest, LossBasedReduction) {
    // High loss — aggressive reduction
    controller_.on_receiver_report(0.10f, 20, 10'000'000);
    EXPECT_LT(controller_.current_bitrate(), 2'000'000);
}

TEST_F(BitrateControllerTest, GoodNetworkIncrease) {
    // Low loss — gradual increase
    uint32_t initial = controller_.current_bitrate();

    for (int i = 0; i < 20; ++i) {
        controller_.on_receiver_report(0.0f, 20, 15'000'000);
    }

    EXPECT_GT(controller_.current_bitrate(), initial);
}

TEST_F(BitrateControllerTest, RembRespected) {
    controller_.on_receiver_report(0.0f, 20, 1'000'000);
    EXPECT_LE(controller_.current_bitrate(), 1'000'000);
}

TEST_F(BitrateControllerTest, MinMaxClamp) {
    // Force below minimum
    for (int i = 0; i < 50; ++i) {
        controller_.on_receiver_report(0.50f, 500, 15'000'000);
    }
    EXPECT_GE(controller_.current_bitrate(), 300'000);

    // Force above maximum
    for (int i = 0; i < 100; ++i) {
        controller_.on_receiver_report(0.0f, 5, 50'000'000);
    }
    EXPECT_LE(controller_.current_bitrate(), 15'000'000);
}

// --- Media Protocol Tests ---

TEST(MediaProtocolTest, HeaderSerialization) {
    MediaPacket packet;
    packet.sequence = 42;
    packet.stream_id = 0; // Video
    packet.timestamp = 90000; // 1 second at 90kHz
    packet.frame_id = 7;
    packet.flags = MediaPacket::FLAG_KEYFRAME | MediaPacket::FLAG_FRAGMENT_START;
    packet.fragment_index = 0;

    auto serialized = packet.serialize();
    EXPECT_EQ(serialized.size(), MediaPacket::HEADER_SIZE);

    auto parsed = MediaPacket::parse(serialized);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->sequence, 42);
    EXPECT_TRUE(parsed->is_keyframe());
    EXPECT_TRUE(parsed->is_fragment_start());
}

TEST(MediaProtocolTest, FragmentReassembly) {
    FrameAssembler assembler;

    // Send 3 fragments
    for (int i = 0; i < 3; ++i) {
        MediaPacket pkt;
        pkt.frame_id = 100;
        pkt.fragment_index = i;
        pkt.fragment_total = 3;
        pkt.payload = std::vector<uint8_t>(100, static_cast<uint8_t>(i));

        if (i == 0) pkt.flags |= MediaPacket::FLAG_FRAGMENT_START;
        if (i == 2) pkt.flags |= MediaPacket::FLAG_FRAGMENT_END;

        assembler.feed(pkt);
    }

    auto frame = assembler.try_assemble();
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->data.size(), 300);
    EXPECT_EQ(frame->frame_id, 100);
}