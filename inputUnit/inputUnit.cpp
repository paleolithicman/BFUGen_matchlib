#include "inputUnit.h"

void inputUnit::inputUnit_main() {
    primate_ctrl_iu::cmd_t cmd;

    // initialize handshake
    bt0 = 0;
    bt1 = 0;
    bt2 = 0;
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
            } else if (cmd.ar_imm == 2) {
                bt2 = cmd.ar_bits;
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
    header_t header_0;
    header_t header_1;
    header_t header_2;
    header_t header_3;
    header_t header_4;
    header_t header_5;
    header_t header_6;
    header_t header_7;
    hdr_count = 0;
    pkt_empty = 0;

    // Input_header<ethernet_t>(14, eth); {
    payload = stream_in.read();
    in_data_buf = payload.data;
    last_buf = payload.last;
    eth.set(payload.data.range(111, 0));
    pkt_empty = 14;
    pkt_data_buf = payload.data.range(511, 112);
    bfu_out.write(tag, bt0, 1, eth.to_uint());
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
        bfu_out.write(tag, bt0, 2, ptp_l.to_uint(), 3, ptp_h.to_uint());
        if (ptp_l.reserved2 == 1) {
            // Input_header<header_t>(8, header_0); {
            payload = stream_in.read();
            header_0.set((payload.data.range(15, 0), in_data_buf.range(511, 464)));
            pkt_empty = 2;
            in_data_buf = payload.data;
            last_buf = payload.last;
            pkt_data_buf = payload.data.range(511, 16);
            // }
            // bfu_out.write(tag, bt0, 4, header_0.to_uint());
            hdr_count = 2;
            if (header_0.field_0 != 0) {
                // Input_header<header_t>(8, header_1); {
                header_1.set(in_data_buf.range(79, 16));
                pkt_empty = 10;
                pkt_data_buf = payload.data.range(511, 80);
                // }
                hdr_count = 3;
                bfu_out.write(tag, bt0, 4, header_0.to_uint(), 5, header_1.to_uint());
                if (header_1.field_0 != 0) {
                    // Input_header<header_t>(8, header_2); {
                    header_2.set(in_data_buf.range(143, 80));
                    pkt_empty = 18;
                    pkt_data_buf = payload.data.range(511, 144);
                    // }
                    // bfu_out.write(tag, bt0, 5, header_2.to_uint());
                    hdr_count = 4;
                    if (header_2.field_0 != 0) {
                        // Input_header<header_t>(8, header_3); {
                        header_3.set(in_data_buf.range(207, 144));
                        pkt_empty = 26;
                        pkt_data_buf = payload.data.range(511, 208);
                        // }
                        bfu_out.write(tag, bt0, 6, header_2.to_uint(), 7, header_3.to_uint());
                        hdr_count = 5;
                        if (header_3.field_0 != 0) {
                            header_4.set(in_data_buf.range(271, 208));
                            pkt_empty = 34;
                            pkt_data_buf = payload.data.range(511, 272);
                            hdr_count = 6;
                            if (header_4.field_0 != 0) {
                                header_5.set(in_data_buf.range(335, 272));
                                pkt_empty = 42;
                                pkt_data_buf = payload.data.range(511, 336);
                                bfu_out.write(tag, bt0, 8, header_4.to_uint(), 9, header_5.to_uint());
                                hdr_count = 7;
                                if (header_5.field_0 != 0) {
                                    header_6.set(in_data_buf.range(399, 336));
                                    pkt_empty = 50;
                                    pkt_data_buf = payload.data.range(511, 400);
                                    hdr_count = 8;
                                    if (header_6.field_0 != 0) {
                                        header_7.set(in_data_buf.range(479, 400));
                                        pkt_empty = 58;
                                        pkt_data_buf = payload.data.range(511, 480);
                                        bfu_out.write(tag, bt0, 10, header_6.to_uint(), 11, header_7.to_uint());
                                        hdr_count = 9;
                                        if (header_7.field_0 != 0) {
                                            payload.data = pkt_data_buf;
                                            payload.empty = pkt_empty;
                                            payload.last = last_buf;
                                            pkt_buf_out.write(payload);
                                            bfu_out.write(tag, bt1, 22, hdr_count, true, true);
                                        }
                                    } else {
                                        bfu_out.write(tag, bt0, 10, header_6.to_uint());
                                    }
                                }
                            } else {
                                bfu_out.write(tag, bt0, 8, header_4.to_uint());
                            }
                        }
                    } else {
                        bfu_out.write(tag, bt0, 6, header_2.to_uint());
                    }
                }
            } else {
                bfu_out.write(tag, bt0, 4, header_0.to_uint());
            }
        }
    } else if (eth.etherType == 0x800) {
        // early exit
        bfu_out.write(tag, bt2, hdr_count, true, true);
        return;
    }

    // Input_done(); {
    wait();
    payload.data = pkt_data_buf;
    payload.empty = pkt_empty;
    payload.last = last_buf;
    pkt_buf_out.write(payload);
    // while (!last_buf) {
    //     payload = stream_in.read();
    //     last_buf = payload.last;
    //     pkt_buf_out.write(payload);
    // }
    bfu_out.write(tag, bt0, 22, hdr_count, true, false);
    // }
}
