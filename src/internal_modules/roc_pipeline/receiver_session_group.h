/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_session_group.h
//! @brief Receiver session group.

#ifndef ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
#define ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_

#include "roc_audio/mixer.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_pipeline/receiver_session_router.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtcp/communicator.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/iparticipant.h"
#include "roc_rtp/identity.h"

namespace roc {
namespace pipeline {

//! Receiver session group.
//!
//! Contains:
//!  - a set of related receiver sessions
//!
//! Session group corresponds to all sessions handled by one receiver slot - a set of
//! related complementary endpoints, e.g. one endpoint for audio, one for repair, and one
//! for control packets.
//!
//! Session group creates and removes sessions and routes packets from endpoints to
//! sessions with the help of ReceiverSessionRouter.
//!
//! It also exchanges control information with remote senders using rtcp::Communicator
//! and updates routing based on that control information.
class ReceiverSessionGroup : public core::NonCopyable<>, private rtcp::IParticipant {
public:
    //! Initialize.
    ReceiverSessionGroup(const ReceiverSourceConfig& source_config,
                         const ReceiverSlotConfig& slot_config,
                         StateTracker& state_tracker,
                         audio::Mixer& mixer,
                         const rtp::EncodingMap& encoding_map,
                         packet::PacketFactory& packet_factory,
                         core::BufferFactory<uint8_t>& byte_buffer_factory,
                         core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                         core::IArena& arena);

    ~ReceiverSessionGroup();

    //! Check if pipeline was succefully constructed.
    bool is_valid() const;

    //! Create control sub-pipeline.
    //! @note
    //!  Control sub-pipeline is shared among all sessions in same group, so
    //!  it's created separately using this method. On the other hand,
    //!  transport sub-pipeline is per-session and is created automatically
    //!  when a session is created within group.
    bool create_control_pipeline(ReceiverEndpoint* control_endpoint);

    //! Route packet to session.
    ROC_ATTR_NODISCARD status::StatusCode route_packet(const packet::PacketPtr& packet,
                                                       core::nanoseconds_t current_time);

    //! Refresh pipeline according to current time.
    //! @returns
    //!  deadline (absolute time) when refresh should be invoked again
    //!  if there are no frames
    core::nanoseconds_t refresh_sessions(core::nanoseconds_t current_time);

    //! Adjust session clock to match consumer clock.
    //! @remarks
    //!  @p playback_time specified absolute time when first sample of last frame
    //!  retrieved from pipeline will be actually played on sink
    void reclock_sessions(core::nanoseconds_t playback_time);

    //! Get number of sessions in group.
    size_t num_sessions() const;

    //! Get slot metrics.
    //! @remarks
    //!  These metrics are for the whole slot.
    //!  For metrics for specific participant, see get_participant_metrics().
    void get_slot_metrics(ReceiverSlotMetrics& slot_metrics) const;

    //! Get metrics for remote participants.
    //! @remarks
    //!  On receiver, one participant corresponds to one ReceiverSession inside
    //!  ReceiverSessionGroup, because we create a separate session for every
    //!  connected participant (remote sender).
    //! @note
    //!  @p party_metrics points to array of metrics structs, and @p party_count
    //!  defines number of array elements. Metrics are written to given array,
    //!  and @p party_count is updated of actual number of elements written.
    //!  If there is not enough space for all metrics, result is truncated.
    void get_participant_metrics(ReceiverParticipantMetrics* party_metrics,
                                 size_t* party_count) const;

private:
    // Implementation of rtcp::IParticipant interface.
    // These methods are invoked by rtcp::Communicator.
    virtual rtcp::ParticipantInfo participant_info();
    virtual void change_source_id();
    virtual size_t num_recv_streams();
    virtual void query_recv_streams(rtcp::RecvReport* reports,
                                    size_t n_reports,
                                    core::nanoseconds_t report_time);
    virtual status::StatusCode notify_recv_stream(packet::stream_source_t send_source_id,
                                                  const rtcp::SendReport& send_report);
    virtual void halt_recv_stream(packet::stream_source_t send_source_id);

    status::StatusCode route_transport_packet_(const packet::PacketPtr& packet);
    status::StatusCode route_control_packet_(const packet::PacketPtr& packet,
                                             core::nanoseconds_t current_time);

    bool can_create_session_(const packet::PacketPtr& packet);

    status::StatusCode create_session_(const packet::PacketPtr& packet);
    void remove_session_(core::SharedPtr<ReceiverSession> sess);
    void remove_all_sessions_();

    ReceiverSessionConfig make_session_config_(const packet::PacketPtr& packet) const;

    const ReceiverSourceConfig source_config_;
    const ReceiverSlotConfig slot_config_;

    StateTracker& state_tracker_;
    audio::Mixer& mixer_;

    const rtp::EncodingMap& encoding_map_;

    core::IArena& arena_;
    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;

    core::Optional<rtp::Identity> identity_;

    core::Optional<rtcp::Communicator> rtcp_communicator_;
    address::SocketAddr rtcp_inbound_addr_;

    core::List<ReceiverSession> sessions_;
    ReceiverSessionRouter session_router_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
