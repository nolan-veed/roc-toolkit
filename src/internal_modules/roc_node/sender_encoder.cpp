/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/sender_encoder.h"
#include "roc_address/interface.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_pipeline/metrics.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace node {

SenderEncoder::SenderEncoder(Context& context,
                             const pipeline::SenderSinkConfig& pipeline_config)
    : Node(context)
    , pipeline_(*this,
                pipeline_config,
                context.encoding_map(),
                context.packet_factory(),
                context.byte_buffer_factory(),
                context.sample_buffer_factory(),
                context.arena())
    , slot_(NULL)
    , processing_task_(pipeline_)
    , valid_(false) {
    roc_log(LogDebug, "sender encoder node: initializing");

    if (!pipeline_.is_valid()) {
        roc_log(LogError, "sender encoder node: failed to construct pipeline");
        return;
    }

    pipeline::SenderSlotConfig slot_config;

    pipeline::SenderLoop::Tasks::CreateSlot slot_task(slot_config);
    if (!pipeline_.schedule_and_wait(slot_task)) {
        roc_log(LogError, "sender encoder node: failed to create slot");
        return;
    }

    slot_ = slot_task.get_handle();
    if (!slot_) {
        roc_log(LogError, "sender encoder node: failed to create slot");
        return;
    }

    valid_ = true;
}

SenderEncoder::~SenderEncoder() {
    roc_log(LogDebug, "sender encoder node: deinitializing");

    if (slot_) {
        // First remove slot. This may involve usage of processing task.
        pipeline::SenderLoop::Tasks::DeleteSlot task(slot_);
        if (!pipeline_.schedule_and_wait(task)) {
            roc_panic("sender encoder node: can't remove pipeline slot");
        }
    }

    // Then wait until processing task is fully completed, before
    // proceeding to its destruction.
    context().control_loop().wait(processing_task_);
}

bool SenderEncoder::is_valid() const {
    return valid_;
}

bool SenderEncoder::activate(address::Interface iface, address::Protocol proto) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "sender encoder node: activating %s interface with protocol %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    if (endpoint_readers_[iface] || endpoint_writers_[iface]) {
        roc_log(LogError,
                "sender encoder node:"
                " can't activate %s interface: interface already activated",
                address::interface_to_str(iface));
        return false;
    }

    endpoint_queues_[iface].reset(new (endpoint_queues_[iface]) packet::ConcurrentQueue(
        packet::ConcurrentQueue::NonBlocking));

    pipeline::SenderLoop::Tasks::AddEndpoint endpoint_task(
        slot_, iface, proto, dest_address_, *endpoint_queues_[iface]);
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "sender encoder node:"
                " can't activate %s interface: can't add endpoint to pipeline",
                address::interface_to_str(iface));
        return false;
    }

    endpoint_readers_[iface] = endpoint_queues_[iface].get();

    if (iface == address::Iface_AudioControl) {
        endpoint_writers_[iface] = endpoint_task.get_inbound_writer();
    }

    return true;
}

bool SenderEncoder::get_metrics(slot_metrics_func_t slot_metrics_func,
                                void* slot_metrics_arg,
                                party_metrics_func_t party_metrics_func,
                                void* party_metrics_arg) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(!slot_metrics_func);
    roc_panic_if(!party_metrics_func);

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderParticipantMetrics party_metrics;
    size_t party_metrics_size = 1;

    pipeline::SenderLoop::Tasks::QuerySlot task(slot_, slot_metrics, &party_metrics,
                                                &party_metrics_size);
    if (!pipeline_.schedule_and_wait(task)) {
        roc_log(LogError,
                "sender encoder node:"
                " can't get metrics: operation failed");
        return false;
    }

    if (slot_metrics_arg) {
        slot_metrics_func(slot_metrics, slot_metrics_arg);
    }

    if (party_metrics_arg) {
        party_metrics_func(party_metrics, 0, party_metrics_arg);
    }

    return true;
}

bool SenderEncoder::is_complete() {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderLoop::Tasks::QuerySlot task(slot_, slot_metrics, NULL, NULL);
    if (!pipeline_.schedule_and_wait(task)) {
        return false;
    }

    return slot_metrics.is_complete;
}

status::StatusCode SenderEncoder::read_packet(address::Interface iface,
                                              packet::PacketPtr& packet) {
    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    packet::IReader* reader = endpoint_readers_[iface];
    if (!reader) {
        roc_log(LogError,
                "sender encoder node:"
                " can't read from %s interface: interface not activated",
                address::interface_to_str(iface));
        // TODO(gh-183): return StatusNotFound
        return status::StatusNoData;
    }

    return reader->read(packet);
}

status::StatusCode SenderEncoder::write_packet(address::Interface iface,
                                               const packet::PacketPtr& packet) {
    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    packet::IWriter* writer = endpoint_writers_[iface];
    if (!writer) {
        if (!endpoint_readers_[iface]) {
            roc_log(LogError,
                    "sender encoder node:"
                    " can't write to %s interface: interface not activated",
                    address::interface_to_str(iface));
            // TODO(gh-183): return StatusNotFound
            return status::StatusUnknown;
        } else {
            roc_log(LogError,
                    "sender encoder node:"
                    " can't write to %s interface: interface doesn't support writing",
                    address::interface_to_str(iface));
            // TODO(gh-183): return StatusBadOperation
            return status::StatusUnknown;
        }
    }

    return writer->write(packet);
}

sndio::ISink& SenderEncoder::sink() {
    roc_panic_if_not(is_valid());

    return pipeline_.sink();
}

void SenderEncoder::schedule_task_processing(pipeline::PipelineLoop&,
                                             core::nanoseconds_t deadline) {
    context().control_loop().schedule_at(processing_task_, deadline, NULL);
}

void SenderEncoder::cancel_task_processing(pipeline::PipelineLoop&) {
    context().control_loop().async_cancel(processing_task_);
}

} // namespace node
} // namespace roc
