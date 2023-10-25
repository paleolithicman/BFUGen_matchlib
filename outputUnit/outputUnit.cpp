#include "outputUnit.h"

void outputUnit::outputUnit_cmd() {
    sc_uint<1> state;
    primate_ctrl_ou::cmd_t cmd;

    // initialize handshake
    cmd_in.reset();
    state = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        hdr_done[i].write(0);
        hdr_arg[i].write(0);
    }
    bt0 = 0;
    bt1 = 0;
    bt2 = 0;
    
    wait();

    while (true) {
        if (done) {
            hdr_done[done_tag.read()].write(0);
        }
        bool cmd_vld = cmd_in.nb_read(cmd);
        if (cmd_vld) {
            opcode = cmd.ar_opcode;
            if (opcode == 0x3f) {
                if (cmd.ar_imm == 0) {
                    bt0 = cmd.ar_bits;
                } else if (cmd.ar_imm == 1) {
                    bt1 = cmd.ar_bits;
                } else if (cmd.ar_imm == 2) {
                    bt2 = cmd.ar_bits;
                }
            } else {
                hdr_arg[cmd.ar_tag].write(cmd.ar_bits);
                hdr_done[cmd.ar_tag].write(1);
            }
        }
        wait();
    }
}

void outputUnit::outputUnit_req() {
    sc_uint<4> state;
    arg_t arg;
    sc_uint<NUM_THREADS_LG> tag;
    primate_stream_512_4::payload_t pkt_in;

    bfu_rdreq.reset();
    state = 0;
    int count = 0;

    wait();

    while (true) {
        // if (state != 0)
        //     cout << sc_time_stamp() << ": req state" << state << endl;
        if (state == 0) {
            pkt_in = pkt_buf_in.peek();
            if (hdr_done[pkt_in.tag].read() == 1) {
                tag = pkt_in.tag;
                arg.set(hdr_arg[pkt_in.tag]);
                state = 1;
            }
            wait();
        } else {
            state = 0;
            outputUnit_req_core(arg.hdr_count, tag);
        }
    }

}

inline void outputUnit::outputUnit_req_core(sc_biguint<32> hdr_count, sc_uint<NUM_THREADS_LG> tag) {
    bfu_rdreq.write(primate_bfu_req_t(tag, 1, 1));
    if (hdr_count < 10) {
        bfu_rdreq.write(primate_bfu_req_t(tag, 2, 3));
        if (hdr_count > 1) {
            if (hdr_count > 2) {
                bfu_rdreq.write(primate_bfu_req_t(tag, 4, 5));
                if (hdr_count > 3) {
                    if (hdr_count > 4) {
                        bfu_rdreq.write(primate_bfu_req_t(tag, 6, 7));
                    } else {
                        bfu_rdreq.write(primate_bfu_req_t(tag, 6, 6));
                    }
                }
            } else {
                bfu_rdreq.write(primate_bfu_req_t(tag, 4, 4));
            }
        }
    }
}

void outputUnit::outputUnit_rsp() {
    sc_uint<4> state;
    arg_t arg;
    sc_uint<NUM_THREADS_LG> tag;
    primate_stream_512_4::payload_t pkt_in;

    bfu_rdrsp.reset();
    stream_out.reset();
    pkt_buf_in.reset();
    bfu_out.reset();
    state = 0;
    done = 0;
    done_tag = 0;

    wait();

    while (true) {
        // cout << sc_time_stamp() << ": rsp state" << state << endl;
        if (state == 0) {
            done = false;
            pkt_in = pkt_buf_in.peek();
            if (hdr_done[pkt_in.tag].read() == 1) {
                tag = pkt_in.tag;
                arg.set(hdr_arg[pkt_in.tag]);
                state = 1;
            }
            wait();
        } else {
            state = 0;
            done = true;
            done_tag = tag;
            outputUnit_rsp_core(arg.hdr_count, tag);
        }
    }

}

inline void outputUnit::outputUnit_rsp_core(sc_biguint<32> hdr_count, sc_uint<NUM_THREADS_LG> tag) {
    sc_biguint<512> out_buf;
    sc_uint<8> empty;
    primate_bfu_rsp_t hdr_data;
    primate_stream_512_4::payload_t pkt_in;

    hdr_data = bfu_rdrsp.read();
    out_buf = hdr_data.data0.range(111, 0);
    empty = 50;
    if (hdr_count < 10) {
        hdr_data = bfu_rdrsp.read();
        out_buf = (hdr_data.data1.range(191, 0), hdr_data.data0.range(159, 0), out_buf.range(111, 0));
        empty = 6;
        if (hdr_count > 1) {
            if (hdr_count > 2) {
                hdr_data = bfu_rdrsp.read();
                out_buf = (hdr_data.data0.range(47, 0), out_buf.range(463, 0));
                empty = 0;
                stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
                out_buf = (hdr_data.data1.range(63, 0), hdr_data.data0.range(63, 48));
                empty = 54;
                if (hdr_count > 3) {
                    if (hdr_count > 4) {
                        hdr_data = bfu_rdrsp.read();
                        out_buf = (hdr_data.data1.range(63, 0), hdr_data.data0.range(63, 0), out_buf.range(79, 0));
                        empty = 38;
                        if (hdr_count > 5) {
                            stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
                            bfu_out.write(tag, bt1);
                            return;
                        }
                    } else {
                        hdr_data = bfu_rdrsp.read();
                        out_buf = (hdr_data.data0.range(63, 0), out_buf.range(79, 0));
                        empty = 46;
                    }
                }
            } else {
                hdr_data = bfu_rdrsp.read();
                out_buf = (hdr_data.data0.range(47, 0), out_buf.range(463, 0));
                empty = 0;
                stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
                out_buf = hdr_data.data0.range(63, 48);
                empty = 62;
            }
        }
    } else {
        stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
        bfu_out.write(tag, bt2);
        return;
    }

    stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
    do {
        pkt_in = pkt_buf_in.read();
        stream_out.write(pkt_in);
    } while (!pkt_in.last);
    bfu_out.write(tag, bt0);
}