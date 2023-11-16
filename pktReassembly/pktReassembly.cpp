#include "pktReassembly.h"

void pktReassembly::pktReassembly_stage0() {
    primate_ctrl_iu::cmd_t cmd;

    // initialize handshake
    stream_in.reset();
    cmd_in.reset();
    // flow_table_write_rsp.reset();
    flow_table_read_req.reset();

    bt0 = 1;
    bt1 = 6;
    bt2 = 11;
    bt3 = 0;
    
    wait();

#pragma hls_pipeline_init_interval 1
#pragma hls_pipeline_stall_mode flush
    while (true) {
        cmd = cmd_in.read();
        sc_uint<NUM_THREADS_LG> tag = cmd.ar_tag;
        sc_uint<OPCODE_WIDTH> opcode = cmd.ar_opcode;
        // if (opcode == 0x3f) {
        //     if (cmd.ar_imm == 0) {
        //         bt0 = cmd.ar_bits;
        //     } else if (cmd.ar_imm == 1) {
        //         bt1 = cmd.ar_bits;
        //     } else if (cmd.ar_imm == 2) {
        //         bt2 = cmd.ar_bits;
        //     } else if (cmd.ar_imm == 3) {
        //         bt3 = cmd.ar_bits;
        //     }
        //     arg.tag = tag;
        //     arg.opcode = 0x3f;
        //     arg.data = cmd_ar_bits;
        //     req_rsp_fifo.write(arg.to_uint());
        //     // bfu_out.write_last(tag, 0);
        // } else if (opcode == 1) {
        //if (opcode == 1) {
        //    output_t out;
        //    out.tag = tag;
        //    out.opcode = opcode;
        //    out.data = cmd.ar_bits;
        //    req_rsp_fifo.write(out.to_uint());
            // unlock_req.write(bfu_in_pl_t(tag, opcode, cmd.ar_imm, cmd.ar_bits));
            // bfu_out.write_last(tag, 0);
        //} else {
            pktReassembly_stage0_core(tag, opcode);
        //}
    }
}

void pktReassembly::pktReassembly_stage0_core(sc_uint<NUM_THREADS_LG> tag, sc_uint<OPCODE_WIDTH> opcode) {
    primate_stream_256_4::payload_t payload;

    meta_t input;
    payload = stream_in.read();
    input.set(payload.data);
    if ((input.tcp_flags == (1 << TCP_FACK)) && (input.len == 0)) {
        // std::cout << "ack packet\n";
        input.pkt_flags = PKT_FORWARD;
        output_t out;
        out.tag = tag;
        out.opcode = 0x3f;
        out.data = input.to_uint();
        req_rsp_fifo.write(out.to_uint());
        // stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
        // bfu_out.write_last(tag, bt0);
        return;
    } else if (input.prot == PROT_UDP) {
        // std::cout << "udp packet\n";
        input.pkt_flags = PKT_CHECK;
        output_t out;
        out.tag = tag;
        out.opcode = 0x3f;
        out.data = input.to_uint();
        req_rsp_fifo.write(out.to_uint());
        // stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
        // bfu_out.write_last(tag, bt0);
        return;
    } else {
        if (input.len != 0) input.pkt_flags = PKT_CHECK;
        else input.pkt_flags = PKT_FORWARD;
        // std::cout << "tcp packet, seq: " << (unsigned)input.seq << ", length: " << input.len << ", flags: " << (unsigned)input.tcp_flags << "\n";
    }
    arg_t arg;
    arg.input = input;
    output_t out;
    out.tag = tag;
    out.opcode = opcode;
    out.data = arg.to_uint();
    req_rsp_fifo.write(out.to_uint());
    flow_table_read_req.write(bfu_in_pl_t(tag, 4, 0, input.to_uint()));
    return;
}

void pktReassembly::pktReassembly_stage1() {
    stream_out.reset();
    bfu_out.reset();
    unlock_req.reset();
    flow_table_write_req.reset();
    flow_table_read_rsp.reset();

    wait();

#pragma hls_pipeline_init_interval 1
#pragma hls_pipeline_stall_mode flush
    while (true) {
        output_t out;
        sc_biguint<NUM_THREADS_LG+OPCODE_WIDTH+252> fifo_out;
        req_rsp_fifo.read(fifo_out);
        out.set(fifo_out);
        if (out.opcode == 0x3f) {
            stream_out.write(primate_io_payload_t(out.data, out.tag, 0, true));
            bfu_out.write_last(out.tag, 2);
        } else if (out.opcode == 1) {
            unlock_req.write(bfu_in_pl_t(out.tag, out.opcode, 0, out.data));
            bfu_out.write_last(out.tag, 0);
        } else {
            arg_t arg;
            arg.set(out.data);
            pktReassembly_stage1_core(out.tag, out.opcode, arg.input);
        }
    }
}

void pktReassembly::pktReassembly_stage1_core(sc_uint<NUM_THREADS_LG> tag, sc_uint<OPCODE_WIDTH> opcode, meta_t input) {
    bfu518_out_pl_t tmp = flow_table_read_rsp.read();
    ftOut_t ftOut;
    ftOut.set(tmp.bits);
    tag = tmp.tag;
    fce_t fte;
    fte = ftOut.fce;
    input = ftOut.pkt;
    if (fte.ch0_bit_map != 0) { // ch0_bit_map shows which sub-table it hits
        // Flow exists
        // std::cout << "flow " << std::hex << input.tuple << std::dec << " exists, seq: " << fte.seq << "\n";
        if (input.seq == fte.seq) {
            if (fte.slow_cnt > 0) {
                bfu_out.write(tag, bt1, 1, input.to_uint(), 2, fte.to_uint(), true, true);
                return;
            } else {
                // in order packet
                // std::cout << std::hex << "tcp flags: " << input.tcp_flags << std::dec << "\n";
                if ((input.tcp_flags & (1 << TCP_FIN) | (input.tcp_flags & (1 << TCP_RST))) != 0) {
                    flow_table_write_req.write(bfu_in_pl_t(tag, 3, 0, fte.to_uint()));
                    // flow_table_write_rsp.read();
                } else {
                    fte.seq = input.seq + input.len;
                    flow_table_write_req.write(bfu_in_pl_t(tag, 2, 0, fte.to_uint()));
                    // flow_table_write_rsp.read();
                }
                unlock_req.write(bfu_in_pl_t(tag, 1, 0, input.to_uint()));
                stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
                bfu_out.write_last(tag, bt0);
                return;
            }
        } else if (input.seq > fte.seq) {
            // insert packet
            bfu_out.write(tag, bt2, 1, input.to_uint(), 2, fte.to_uint(), true, true);
            return;
        } else {
            // drop the packet
            input.pkt_flags = PKT_DROP;
            unlock_req.write(bfu_in_pl_t(tag, 1, 0, input.to_uint()));
            stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
            bfu_out.write_last(tag, bt0);
            return;
        }
    } else {
        // Flow doesn't exist, insert
        // std::cout << "new flow " << std::hex << input.tuple << std::dec << "\n";
        fte.ch0_bit_map = 0x10;

        if (((input.tcp_flags & (1 << TCP_FIN)) | (input.tcp_flags & (1 << TCP_RST))) == 0) {
            flow_table_write_req.write(bfu_in_pl_t(tag, 1, 0, fte.to_uint()));
        }
        // flow_table_write_rsp.read();
        unlock_req.write(bfu_in_pl_t(tag, 1, 0, input.to_uint()));
        stream_out.write(primate_io_payload_t(input.to_uint(), tag, 0, true));
        bfu_out.write_last(tag, bt0);
        return;
    }
}
