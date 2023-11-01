#include "outputUnit.h"

void outputUnit::outputUnit_req() {
    sc_uint<4> state;
    arg_t arg;
    sc_uint<NUM_THREADS_LG> tag;
    primate_ctrl_ou::cmd_t cmd;

    bfu_rdreq.reset();
    cmd_in.reset();
    state = 0;

    wait();

    while (true) {
        // if (state != 0)
        //     cout << sc_time_stamp() << ": req state" << state << endl;
        if (state == 0) {
            cmd = cmd_in.read();
            tag = cmd.ar_tag;
            arg.set(cmd.ar_bits);
            req_rsp_fifo.write((cmd.ar_bits, tag));
            state = 1;
            // wait();
        } else {
            state = 0;
            outputUnit_req_core(arg.hdr_count, tag);
        }
    }

}

inline void outputUnit::outputUnit_req_core(sc_biguint<32> hdr_count, sc_uint<NUM_THREADS_LG> tag) {
    bfu_rdreq.write(primate_bfu_req_t(tag, 1, 1));
    if (hdr_count == 1) {
        bfu_rdreq.write(primate_bfu_req_t(tag, 2, 3));
    } else if (hdr_count == 2 || hdr_count == 3) {
        bfu_rdreq.write(primate_bfu_req_t(tag, 2, 3));
        bfu_rdreq.write(primate_bfu_req_t(tag, 4, 4));
    } else if (hdr_count == 4 || hdr_count == 5) {
        bfu_rdreq.write(primate_bfu_req_t(tag, 2, 3));
        bfu_rdreq.write(primate_bfu_req_t(tag, 4, 5));
    } else if (hdr_count == 6 || hdr_count == 7) {
        bfu_rdreq.write(primate_bfu_req_t(tag, 2, 3));
        bfu_rdreq.write(primate_bfu_req_t(tag, 4, 5));
        bfu_rdreq.write(primate_bfu_req_t(tag, 6, 6));
    } else if (hdr_count >= 8) {
        bfu_rdreq.write(primate_bfu_req_t(tag, 2, 3));
        bfu_rdreq.write(primate_bfu_req_t(tag, 4, 5));
        bfu_rdreq.write(primate_bfu_req_t(tag, 6, 7));
    }
}

void outputUnit::outputUnit_rsp() {
    sc_uint<4> state;
    arg_t arg;
    sc_biguint<32+NUM_THREADS_LG> fifo_out;
    sc_uint<NUM_THREADS_LG> tag;

    bfu_rdrsp.reset();
    stream_out.reset();
    bfu_out.reset();
    state = 0;

    wait();

    while (true) {
        // cout << sc_time_stamp() << ": rsp state" << state << endl;
        if (state == 0) {
            req_rsp_fifo.read(fifo_out);
            tag = fifo_out.range(NUM_THREADS_LG-1, 0);
            arg.set(fifo_out.range(31+NUM_THREADS_LG, NUM_THREADS_LG));
            state = 1;
            wait();
        } else {
            state = 0;
            outputUnit_rsp_core(arg.hdr_count, tag);
        }
    }

}

inline void outputUnit::outputUnit_rsp_core(sc_biguint<32> hdr_count, sc_uint<NUM_THREADS_LG> tag) {
    sc_biguint<512> out_buf;
    sc_uint<8> empty;
    primate_bfu_rsp_t hdr_data;

    hdr_data = bfu_rdrsp.read();
    out_buf = hdr_data.data0.range(111, 0);
    empty = 50;
    if (hdr_count < 10) {
        hdr_data = bfu_rdrsp.read();
        out_buf = (hdr_data.data1.range(191, 0), hdr_data.data0.range(159, 0), out_buf.range(111, 0));
        empty = 6;
        if (hdr_count > 1) {
            if (hdr_count > 2) {
                if (hdr_count > 3) {
                    if (hdr_count > 4) {
                        hdr_data = bfu_rdrsp.read();
                        out_buf = (hdr_data.data0.range(47, 0), out_buf.range(463, 0));
                        empty = 0;
                        stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
                        out_buf = (hdr_data.data1.range(127, 0), hdr_data.data0.range(127, 48));
                        empty = 38;
                        if (hdr_count > 5) {
                            if (hdr_count > 6) {
                                if (hdr_count > 7) {
                                    if (hdr_count > 8) {
                                        hdr_data = bfu_rdrsp.read();
                                        out_buf = (hdr_data.data1.range(127, 0), hdr_data.data0.range(127, 0), out_buf.range(207, 0));
                                        empty = 8;
                                    } else {
                                        hdr_data = bfu_rdrsp.read();
                                        out_buf = (hdr_data.data1.range(63, 0), hdr_data.data0.range(127, 0), out_buf.range(207, 0));
                                        empty = 16;
                                    }
                                } else {
                                    hdr_data = bfu_rdrsp.read();
                                    out_buf = (hdr_data.data0.range(127, 0), out_buf.range(207, 0));
                                    empty = 24;
                                }
                            } else {
                                hdr_data = bfu_rdrsp.read();
                                out_buf = (hdr_data.data0.range(63, 0), out_buf.range(207, 0));
                                empty = 30;
                            }

                        }
                    } else {
                        hdr_data = bfu_rdrsp.read();
                        out_buf = (hdr_data.data0.range(47, 0), out_buf.range(463, 0));
                        empty = 0;
                        stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
                        out_buf = (hdr_data.data1.range(63, 0), hdr_data.data0.range(127, 48));
                        empty = 46;
                    }
                } else {
                    hdr_data = bfu_rdrsp.read();
                    out_buf = (hdr_data.data0.range(47, 0), out_buf.range(463, 0));
                    empty = 0;
                    stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
                    out_buf = hdr_data.data0.range(127, 48);
                    empty = 54;
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
        bfu_out.write(tag, bt1.read(), true);
        return;
    }

    stream_out.write(primate_io_payload_t(out_buf, tag, empty, false));
// #pragma hls_pipeline_init_interval 1
//     do {
//         pkt_in = pkt_buf_in.read();
//         stream_out.write(pkt_in);
//     } while (!pkt_in.last);
    bfu_out.write(tag, bt0.read());
}