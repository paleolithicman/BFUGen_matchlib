#include "inputUnit.h"

void inputUnit::inputUnit_main() {
    primate_ctrl_iu::cmd_t cmd;

    // initialize handshake
    bt0 = 0;
    bt1 = 0;
    stream_in.reset();
    cmd_in.reset();
    pkt_buf_out.reset();
    bfu_out.reset();
    
    wait();

    // main FSM
// #pragma hls_pipeline_init_interval 1
// #pragma hls_pipeline_stall_mode flush
    while (true) {
        cmd = cmd_in.read();
        tag = cmd.ar_tag;
        opcode = cmd.ar_opcode;
        if (opcode == 0x3f) {
            if (cmd.ar_imm == 0) {
                bt0 = cmd.ar_bits;
            } else if (cmd.ar_imm == 1) {
                bt1 = cmd.ar_bits;
            }
            bfu_out.write_last(tag, 0);
        } else {
            inputUnit_core();
        }
    }
}

void inputUnit::inputUnit_core() {
    primate_stream_512_4::payload_t payload;
    sc_biguint<512> in_data_buf;
    sc_biguint<512> pkt_data_buf;
    bool last_buf;
    int path;

    sc_biguint<4> hdr_count;
    sc_uint<8> pkt_empty;
    ethernet_t eth;
    ptp_l_t ptp_l;
    ptp_h_t ptp_h;
    header2_t header_0;
    header2_t header_1;
    header2_t header_2;
    header2_t header_3;
    hdr_count = 0;
    pkt_empty = 0;

    // Input_header<ethernet_t>(14, eth); {
    payload = stream_in.read();
    in_data_buf = payload.data;
    last_buf = payload.last;
    eth.set(payload.data.range(111, 0));
    pkt_empty = 14;
    pkt_data_buf = payload.data.range(511, 112);
    // }
    if (eth.etherType == 0x88f7) {
        // Input_header<ptp_l_t>(20, ptp_l); {
        ptp_l.set(payload.data.range(271, 112));
        pkt_empty = 34;
        // }
        // Input_header<ptp_h_t>(24, ptp_h); {
        ptp_h.set(payload.data.range(463, 272));
        pkt_empty = 58;
        pkt_data_buf = payload.data.range(511, 464);
        // }
        hdr_count = 1;
        if (ptp_l.reserved2 == 1) {
            // Input_header<header_t>(8, header_0); {
            payload = stream_in.read();
            header_0.hdr0.set((payload.data.range(15, 0), in_data_buf.range(511, 464)));
            pkt_empty = 2;
            in_data_buf = payload.data;
            last_buf = payload.last;
            pkt_data_buf = payload.data.range(511, 16);
            // }
            hdr_count = 2;
            if (header_0.hdr0.field_0 != 0) {
                // Input_header<header_t>(8, header_1); {
                header_0.hdr1.set(in_data_buf.range(79, 16));
                pkt_empty = 10;
                pkt_data_buf = in_data_buf.range(511, 80);
                // }
                hdr_count = 3;
                if (header_0.hdr1.field_0 != 0) {
                    header_1.hdr0.set(in_data_buf.range(143, 80));
                    pkt_empty = 18;
                    pkt_data_buf = in_data_buf.range(511, 144);
                    // }
                    hdr_count = 4;
                    if (header_1.hdr0.field_0 != 0) {
                        // Input_header<header_t>(8, header_3); {
                        header_1.hdr1.set(in_data_buf.range(207, 144));
                        pkt_empty = 26;
                        pkt_data_buf = in_data_buf.range(511, 208);
                        // }
                        hdr_count = 5;
                        if (header_1.hdr1.field_0 != 0) {
                            header_2.hdr0.set(in_data_buf.range(271, 208));
                            pkt_empty = 34;
                            pkt_data_buf = in_data_buf.range(511, 272);
                            hdr_count = 6;
                            if (header_2.hdr0.field_0 != 0) {
                                header_2.hdr1.set(in_data_buf.range(335, 272));
                                pkt_empty = 42;
                                pkt_data_buf = in_data_buf.range(511, 336);
                                hdr_count = 7;
                                if (header_2.hdr1.field_0 != 0) {
                                    header_3.hdr0.set(in_data_buf.range(399, 336));
                                    pkt_empty = 50;
                                    pkt_data_buf = in_data_buf.range(511, 400);
                                    hdr_count = 8;
                                    if (header_3.hdr0.field_0 != 0) {
                                        header_3.hdr1.set(in_data_buf.range(463, 400));
                                        pkt_empty = 58;
                                        pkt_data_buf = in_data_buf.range(511, 464);
                                        hdr_count = 9;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (eth.etherType == 0x800) {
        // early exit
        bfu_out.write(tag, bt1, 1, eth.to_uint(), 22, hdr_count, true, true);
        return;
    }

    // Input_done(); {
    wait();
    payload.data = pkt_data_buf;
    payload.empty = pkt_empty;
    payload.last = last_buf;
    pkt_buf_out.write(payload);
#pragma hls_pipeline_init_interval 1
    while (!last_buf) {
        payload = stream_in.read();
        last_buf = payload.last;
        pkt_buf_out.write(payload);
    }
    bfu_out.write(tag, bt0, 1, eth.to_uint(), 22, hdr_count);
    bfu_out.write(tag, bt0, 2, ptp_l.to_uint(), 3, ptp_h.to_uint());
    bfu_out.write(tag, bt0, 4, header_0.to_uint(), 5, header_1.to_uint());
    bfu_out.write(tag, bt0, 6, header_2.to_uint(), 7, header_3.to_uint(), true, false);
    // }
}
