/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/shipper.h"

namespace roc {
namespace packet {

Shipper::Shipper(IComposer& composer,
                 IWriter& outbound_writer,
                 const address::SocketAddr* outbound_address)
    : composer_(composer)
    , outbound_writer_(outbound_writer) {
    if (outbound_address) {
        outbound_address_ = *outbound_address;
    }
}

const address::SocketAddr& Shipper::outbound_address() const {
    return outbound_address_;
}

status::StatusCode Shipper::write(const PacketPtr& packet) {
    if (outbound_address_) {
        if (!packet->has_flags(Packet::FlagUDP)) {
            packet->add_flags(Packet::FlagUDP);
        }
        if (!packet->udp()->dst_addr) {
            packet->udp()->dst_addr = outbound_address_;
        }
    }

    if (!packet->has_flags(packet::Packet::FlagPrepared)) {
        roc_panic("shipper: unexpected packet: should be prepared");
    }

    if (!packet->has_flags(packet::Packet::FlagComposed)) {
        if (!composer_.compose(*packet)) {
            // TODO(gh-183): return status from composer
            roc_panic("shipper: can't compose packet");
        }
        packet->add_flags(Packet::FlagComposed);
    }

    return outbound_writer_.write(packet);
}

} // namespace packet
} // namespace roc
