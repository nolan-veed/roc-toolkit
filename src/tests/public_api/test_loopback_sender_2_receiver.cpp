/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/context.h"
#include "test_helpers/proxy.h"
#include "test_helpers/receiver.h"
#include "test_helpers/sender.h"

#include "roc_core/time.h"
#include "roc_fec/codec_map.h"

#include "roc/config.h"

namespace roc {
namespace api {

namespace {

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, test::MaxBufSize);

} // namespace

TEST_GROUP(loopback_sender_2_receiver) {
    roc_sender_config sender_conf;
    roc_receiver_config receiver_conf;

    float sample_step;

    void setup() {
        sample_step = 1. / 32768.;
    }

    void init_config(unsigned flags, unsigned frame_chans, unsigned packet_chans,
                     int encoding_id = 0) {
        memset(&sender_conf, 0, sizeof(sender_conf));
        sender_conf.frame_encoding.rate = test::SampleRate;
        sender_conf.frame_encoding.format = ROC_FORMAT_PCM_FLOAT32;

        if (flags & test::FlagMultitrack) {
            sender_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MULTITRACK;
            sender_conf.frame_encoding.tracks = frame_chans;
        } else {
            switch (frame_chans) {
            case 1:
                sender_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MONO;
                break;
            case 2:
                sender_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
                break;
            default:
                FAIL("unexpected frame_chans");
            }
            switch (packet_chans) {
            case 1:
                sender_conf.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_MONO;
                break;
            case 2:
                sender_conf.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;
                break;
            default:
                FAIL("unexpected packet_chans");
            }
        }

        if (encoding_id != 0) {
            sender_conf.packet_encoding = (roc_packet_encoding)encoding_id;
        }

        sender_conf.packet_length =
            test::PacketSamples * 1000000000ull / test::SampleRate;
        sender_conf.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

        if (flags & test::FlagRS8M) {
            sender_conf.fec_encoding = ROC_FEC_ENCODING_RS8M;
            sender_conf.fec_block_source_packets = test::SourcePackets;
            sender_conf.fec_block_repair_packets = test::RepairPackets;
        } else if (flags & test::FlagLDPC) {
            sender_conf.fec_encoding = ROC_FEC_ENCODING_LDPC_STAIRCASE;
            sender_conf.fec_block_source_packets = test::SourcePackets;
            sender_conf.fec_block_repair_packets = test::RepairPackets;
        } else {
            sender_conf.fec_encoding = ROC_FEC_ENCODING_DISABLE;
        }

        memset(&receiver_conf, 0, sizeof(receiver_conf));
        receiver_conf.frame_encoding.rate = test::SampleRate;
        receiver_conf.frame_encoding.format = ROC_FORMAT_PCM_FLOAT32;

        if (flags & test::FlagMultitrack) {
            receiver_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MULTITRACK;
            receiver_conf.frame_encoding.tracks = frame_chans;
        } else {
            switch (frame_chans) {
            case 1:
                receiver_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MONO;
                break;
            case 2:
                receiver_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
                break;
            default:
                FAIL("unexpected frame_chans");
            }
        }

        receiver_conf.clock_source = ROC_CLOCK_SOURCE_INTERNAL;
        receiver_conf.latency_tuner_profile = ROC_LATENCY_TUNER_PROFILE_INTACT;
        receiver_conf.target_latency = test::Latency * 1000000000ull / test::SampleRate;
        receiver_conf.no_playback_timeout =
            test::Timeout * 1000000000ull / test::SampleRate;
    }

    bool is_rs8m_supported() {
        return fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8);
    }

    bool is_ldpc_supported() {
        return fec::CodecMap::instance().is_supported(packet::FEC_LDPC_Staircase);
    }
};

TEST(loopback_sender_2_receiver, bare_rtp) {
    enum { Flags = 0, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, rtp_rtcp) {
    enum { Flags = test::FlagRTCP, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(),
                   receiver.control_endpoint());

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, rs8m_without_losses) {
    if (!is_rs8m_supported()) {
        return;
    }

    enum { Flags = test::FlagRS8M, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, rs8m_with_losses) {
    if (!is_rs8m_supported()) {
        return;
    }

    enum { Flags = test::FlagRS8M, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Proxy proxy(receiver.source_endpoint(), receiver.repair_endpoint(),
                      test::SourcePackets, test::RepairPackets, arena, packet_factory,
                      byte_buffer_factory);

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(proxy.source_endpoint(), proxy.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, ldpc_without_losses) {
    if (!is_ldpc_supported()) {
        return;
    }

    enum { Flags = test::FlagLDPC, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, ldpc_with_losses) {
    if (!is_ldpc_supported()) {
        return;
    }

    enum { Flags = test::FlagLDPC, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Proxy proxy(receiver.source_endpoint(), receiver.repair_endpoint(),
                      test::SourcePackets, test::RepairPackets, arena, packet_factory,
                      byte_buffer_factory);

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(proxy.source_endpoint(), proxy.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, separate_context) {
    enum { Flags = 0, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context recv_context, send_context;

    test::Receiver receiver(recv_context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(send_context, sender_conf, sample_step, FrameChans,
                        test::FrameSamples, Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, multiple_senders_one_receiver_sequential) {
    enum { Flags = 0, FrameChans = 2, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender_1(context, sender_conf, sample_step, FrameChans,
                          test::FrameSamples, Flags);

    sender_1.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender_1.start());
    receiver.receive();
    sender_1.stop();
    sender_1.join();

    receiver.wait_zeros(test::TotalSamples / 2);

    test::Sender sender_2(context, sender_conf, sample_step, FrameChans,
                          test::FrameSamples, Flags);

    sender_2.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender_2.start());
    receiver.receive();
    sender_2.stop();
    sender_2.join();
}

TEST(loopback_sender_2_receiver, sender_slots) {
    enum { Flags = 0, FrameChans = 2, PacketChans = 2, Slot1 = 1, Slot2 = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver_1(context, receiver_conf, sample_step, FrameChans,
                              test::FrameSamples, Flags);

    receiver_1.bind(Flags);

    test::Receiver receiver_2(context, receiver_conf, sample_step, FrameChans,
                              test::FrameSamples, Flags);

    receiver_2.bind(Flags);

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver_1.source_endpoint(), receiver_1.repair_endpoint(), NULL,
                   Slot1);
    sender.connect(receiver_2.source_endpoint(), receiver_2.repair_endpoint(), NULL,
                   Slot2);

    CHECK(sender.start());

    CHECK(receiver_1.start());
    CHECK(receiver_2.start());
    receiver_2.join();
    receiver_1.join();

    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, receiver_slots_sequential) {
    enum { Flags = 0, FrameChans = 2, PacketChans = 2, Slot1 = 1, Slot2 = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind(Slot1);
    receiver.bind(Slot2);

    test::Sender sender_1(context, sender_conf, sample_step, FrameChans,
                          test::FrameSamples, Flags);

    sender_1.connect(receiver.source_endpoint(Slot1), receiver.repair_endpoint(Slot1),
                     NULL);

    CHECK(sender_1.start());
    receiver.receive();
    sender_1.stop();
    sender_1.join();

    receiver.wait_zeros(test::TotalSamples / 2);

    test::Sender sender_2(context, sender_conf, sample_step, FrameChans,
                          test::FrameSamples, Flags);

    sender_2.connect(receiver.source_endpoint(Slot2), receiver.repair_endpoint(Slot2),
                     NULL);

    CHECK(sender_2.start());
    receiver.receive();
    sender_2.stop();
    sender_2.join();
}

TEST(loopback_sender_2_receiver, mono) {
    enum { Flags = 0, FrameChans = 1, PacketChans = 1 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, stereo_mono_stereo) {
    enum { Flags = 0, FrameChans = 2, PacketChans = 1 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, mono_stereo_mono) {
    enum { Flags = 0, FrameChans = 1, PacketChans = 2 };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, multitrack) {
    enum {
        Flags = test::FlagMultitrack,
        FrameChans = 4,
        PacketChans = 4,
        EncodingID = 100
    };

    init_config(Flags, FrameChans, PacketChans, EncodingID);

    test::Context context;

    context.register_multitrack_encoding(EncodingID, PacketChans);

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, multitrack_separate_contexts) {
    enum {
        Flags = test::FlagMultitrack,
        FrameChans = 4,
        PacketChans = 4,
        EncodingID = 100
    };

    init_config(Flags, FrameChans, PacketChans, EncodingID);

    test::Context recv_context, send_context;

    recv_context.register_multitrack_encoding(EncodingID, PacketChans);
    send_context.register_multitrack_encoding(EncodingID, PacketChans);

    test::Receiver receiver(recv_context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(send_context, sender_conf, sample_step, FrameChans,
                        test::FrameSamples, Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(), NULL);

    CHECK(sender.start());
    receiver.receive();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, metrics_measurements) {
    enum {
        Flags = test::FlagNonStrict | test::FlagInfinite | test::FlagRTCP,
        FrameChans = 2,
        PacketChans = 2,
        MaxSess = 10
    };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(), receiver.repair_endpoint(),
                   receiver.control_endpoint());

    {
        receiver.query_metrics(MaxSess);

        UNSIGNED_LONGS_EQUAL(0, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, receiver.conn_metrics_count());
    }

    CHECK(sender.start());
    CHECK(receiver.start());

    for (;;) {
        core::sleep_for(core::ClockMonotonic, core::Millisecond);

        receiver.query_metrics(MaxSess);

        if (receiver.recv_metrics().connection_count == 0) {
            continue;
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, receiver.conn_metrics_count());

        if (receiver.conn_metrics(0).e2e_latency == 0) {
            continue;
        }

        sender.query_metrics(MaxSess);

        if (sender.send_metrics().connection_count == 0) {
            continue;
        }

        UNSIGNED_LONGS_EQUAL(1, sender.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, sender.conn_metrics_count());

        if (sender.conn_metrics(0).e2e_latency == 0) {
            continue;
        }

        break;
    }

    receiver.stop();
    receiver.join();
    sender.stop();
    sender.join();
}

TEST(loopback_sender_2_receiver, metrics_connections) {
    enum {
        Flags = test::FlagNonStrict | test::FlagInfinite | test::FlagRTCP,
        FrameChans = 2,
        PacketChans = 2,
        MaxSess = 10
    };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind();

    test::Sender sender_1(context, sender_conf, sample_step, FrameChans,
                          test::FrameSamples, Flags);

    sender_1.connect(receiver.source_endpoint(), receiver.repair_endpoint(),
                     receiver.control_endpoint());

    test::Sender sender_2(context, sender_conf, sample_step, FrameChans,
                          test::FrameSamples, Flags);

    sender_2.connect(receiver.source_endpoint(), receiver.repair_endpoint(),
                     receiver.control_endpoint());

    {
        receiver.query_metrics(MaxSess);

        UNSIGNED_LONGS_EQUAL(0, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, receiver.conn_metrics_count());
    }

    {
        sender_1.query_metrics(MaxSess);

        UNSIGNED_LONGS_EQUAL(0, sender_1.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, sender_1.conn_metrics_count());
    }

    {
        sender_2.query_metrics(MaxSess);

        UNSIGNED_LONGS_EQUAL(0, sender_2.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, sender_2.conn_metrics_count());
    }

    CHECK(sender_1.start());
    CHECK(sender_2.start());
    CHECK(receiver.start());

    for (;;) {
        core::sleep_for(core::ClockMonotonic, core::Millisecond);

        receiver.query_metrics(MaxSess);

        if (receiver.recv_metrics().connection_count != 2) {
            continue;
        }

        UNSIGNED_LONGS_EQUAL(2, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(2, receiver.conn_metrics_count());

        break;
    }

    for (;;) {
        core::sleep_for(core::ClockMonotonic, core::Millisecond);

        sender_1.query_metrics(MaxSess);

        if (sender_1.send_metrics().connection_count != 1) {
            continue;
        }

        UNSIGNED_LONGS_EQUAL(1, sender_1.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, sender_1.conn_metrics_count());

        break;
    }

    for (;;) {
        core::sleep_for(core::ClockMonotonic, core::Millisecond);

        sender_2.query_metrics(MaxSess);

        if (sender_2.send_metrics().connection_count != 1) {
            continue;
        }

        UNSIGNED_LONGS_EQUAL(1, sender_2.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, sender_2.conn_metrics_count());

        break;
    }

    receiver.stop();
    receiver.join();
    sender_1.stop();
    sender_1.join();
    sender_2.stop();
    sender_2.join();
}

TEST(loopback_sender_2_receiver, metrics_slots) {
    enum {
        Flags = test::FlagNonStrict | test::FlagInfinite | test::FlagRTCP,
        FrameChans = 2,
        PacketChans = 2,
        MaxSess = 10,
        Slot1 = 1,
        Slot2 = 2
    };

    init_config(Flags, FrameChans, PacketChans);

    test::Context context;

    test::Receiver receiver(context, receiver_conf, sample_step, FrameChans,
                            test::FrameSamples, Flags);

    receiver.bind(Slot1);
    receiver.bind(Slot2);

    test::Sender sender(context, sender_conf, sample_step, FrameChans, test::FrameSamples,
                        Flags);

    sender.connect(receiver.source_endpoint(Slot1), receiver.repair_endpoint(Slot1),
                   receiver.control_endpoint(Slot1), Slot1);

    sender.connect(receiver.source_endpoint(Slot2), receiver.repair_endpoint(Slot2),
                   receiver.control_endpoint(Slot2), Slot2);

    {
        receiver.query_metrics(MaxSess, Slot1);

        UNSIGNED_LONGS_EQUAL(0, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, receiver.conn_metrics_count());

        receiver.query_metrics(MaxSess, Slot2);

        UNSIGNED_LONGS_EQUAL(0, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, receiver.conn_metrics_count());
    }

    {
        sender.query_metrics(MaxSess, Slot1);

        UNSIGNED_LONGS_EQUAL(0, sender.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, sender.conn_metrics_count());

        sender.query_metrics(MaxSess, Slot2);

        UNSIGNED_LONGS_EQUAL(0, sender.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(0, sender.conn_metrics_count());
    }

    CHECK(sender.start());
    CHECK(receiver.start());

    for (;;) {
        core::sleep_for(core::ClockMonotonic, core::Millisecond);

        receiver.query_metrics(MaxSess, Slot1);

        if (receiver.recv_metrics().connection_count == 0) {
            continue;
        }

        receiver.query_metrics(MaxSess, Slot2);

        if (receiver.recv_metrics().connection_count == 0) {
            continue;
        }

        break;
    }

    {
        receiver.query_metrics(MaxSess, Slot1);

        UNSIGNED_LONGS_EQUAL(1, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, receiver.conn_metrics_count());

        receiver.query_metrics(MaxSess, Slot2);

        UNSIGNED_LONGS_EQUAL(1, receiver.recv_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, receiver.conn_metrics_count());
    }

    for (;;) {
        core::sleep_for(core::ClockMonotonic, core::Millisecond);

        sender.query_metrics(MaxSess, Slot1);

        if (sender.send_metrics().connection_count == 0) {
            continue;
        }

        sender.query_metrics(MaxSess, Slot2);

        if (sender.send_metrics().connection_count == 0) {
            continue;
        }

        break;
    }

    {
        sender.query_metrics(MaxSess, Slot1);

        UNSIGNED_LONGS_EQUAL(1, sender.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, sender.conn_metrics_count());

        sender.query_metrics(MaxSess, Slot2);

        UNSIGNED_LONGS_EQUAL(1, sender.send_metrics().connection_count);
        UNSIGNED_LONGS_EQUAL(1, sender.conn_metrics_count());
    }

    receiver.stop();
    receiver.join();
    sender.stop();
    sender.join();
}

} // namespace api
} // namespace roc
