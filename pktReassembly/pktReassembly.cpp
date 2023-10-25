#include "pktReassembly.h"

void pktReassembly::pktReassembly_main() {
    primate_ctrl_iu::cmd_t cmd;

    // initialize handshake
    stream_in.reset();
    stream_out.reset();
    cmd_in.reset();
    bfu_out.reset();
    lock_req.reset();
    lock_rsp.reset();
    flow_table_write_req.reset();
    flow_table_write_rsp.reset();
    flow_table_read_req.reset();
    flow_table_read_rsp.reset();

    bt0 = 0;
    bt1 = 0;
    bt2 = 0;
    bt3 = 0;
    
    wait();

    while (true) {
        cmd = cmd_in.read();
        tag = cmd.ar_tag;
        opcode = cmd.ar_opcode;
        if (opcode == 0x3f) {
            if (cmd.ar_imm == 0) {
                bt0 = cmd.ar_bits;
            } else if (cmd.ar_imm == 1) {
                bt1 = cmd.ar_bits;
            } else if (cmd.ar_imm == 2) {
                bt2 = cmd.ar_bits;
            } else if (cmd.ar_imm == 3) {
                bt3 = cmd.ar_bits;
            }
            bfu_out.write_last(tag, 0);
        } else if (opcode & 0x30 == 0x10) {
            lock_req.write(bfu_in_pl_t(tag, opcode, cmd.ar_imm, cmd.ar_bits));
            bfu_out_pl_t tmp = lock_rsp.read();
            bfu_out.write_last(tag, 0);
        } else {
            pktReassembly_core();
        }
    }
}

void pktReassembly::pktReassembly_core() {
    primate_stream_272_4::payload_t payload;

    fce_t fte;
    meta_t input;
    payload = stream_in.read();
    input.set(payload.data);
    if ((input.tcp_flags == (1 << TCP_FACK)) && (input.len == 0)) {
        // std::cout << "ack packet\n";
        input.pkt_flags = PKT_FORWARD;
        stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
        bfu_out.write_last(tag, bt0);
        return;
    } else if (input.prot == PROT_UDP) {
        // std::cout << "udp packet\n";
        input.pkt_flags = PKT_CHECK;
        stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
        bfu_out.write_last(tag, bt0);
        return;
    } else {
        if (input.len != 0) input.pkt_flags = PKT_CHECK;
        else input.pkt_flags = PKT_FORWARD;
        // std::cout << "tcp packet, seq: " << (unsigned)input.seq << ", length: " << input.len << ", flags: " << (unsigned)input.tcp_flags << "\n";
        lock_req.write(bfu_in_pl_t(tag, 0, 0, input.tuple));
        lock_rsp.read();
        flow_table_read_req.write(bfu_in_pl_t(tag, 0, 0, input.to_uint()));
        bfu_out_pl_t tmp = flow_table_read_rsp.read();
        fte.set(tmp.bits);
        if (fte.ch0_bit_map != 0) { // ch0_bit_map shows which sub-table it hits
            // Flow exists
            // std::cout << "flow " << std::hex << input.tuple << std::dec << " exists, seq: " << fte.seq << "\n";
            if (input.seq == fte.seq) {
                if (fte.slow_cnt > 0) {
                    bfu_out.write(tag, bt1, 1, input.to_uint(), 2, fte.to_uint(), true);
                    return;
                } else {
                    // in order packet
                    // std::cout << std::hex << "tcp flags: " << input.tcp_flags << std::dec << "\n";
                    if ((input.tcp_flags & (1 << TCP_FIN) | (input.tcp_flags & (1 << TCP_RST))) != 0) {
                        flow_table_write_req.write(bfu_in_pl_t(tag, 3, 0, fte.to_uint()));
                        flow_table_write_rsp.read();
                    } else {
                        fte.seq = input.seq + input.len;
                        flow_table_write_req.write(bfu_in_pl_t(tag, 2, 0, fte.to_uint()));
                        flow_table_write_rsp.read();
                    }
                    lock_req.write(bfu_in_pl_t(tag, 1, 0, input.tuple));
                    lock_rsp.read();
                    stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
                    bfu_out.write_last(tag, bt0);
                    return;
                }
            } else if (input.seq > fte.seq) {
                // insert packet
                bfu_out.write(tag, bt2, 1, input.to_uint(), 2, fte.to_uint(), true);
                return;
            } else {
                bfu_out.write(tag, bt3, 1, input.to_uint(), true);
                return;
            }
        } else {
            // Flow doesn't exist, insert
            // std::cout << "new flow " << std::hex << input.tuple << std::dec << "\n";
            flow_table_write_req.write(bfu_in_pl_t(tag, 1, 0, input.to_uint()));
            flow_table_write_rsp.read();
            lock_req.write(bfu_in_pl_t(tag, 1, 0, input.tuple));
            lock_rsp.read();
            stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
            bfu_out.write_last(tag, bt0);
            return;
        }
    }
}
