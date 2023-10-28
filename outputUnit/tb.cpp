#include "outputUnit.h"
#include <map>
#define EMPTY 26
#define HDR_COUNT 9
#define NUM_PKT 2
using namespace std;

string pkt_data[3] = {
    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "00000000f0000000e0000000d0000000c0000000b0000000a00000009000000080000000700000006000000050000000400000003000000020000000100000000",
    "00000000f0000000e0000000d0000000c0000000b0000000a00000009000000080000000700000006000000050000000400000003000000020000000100000000"
};

string regfile_data[16] = {
    "0000000000000000000088f700000000beef00000000dead",
    "0000000000000000000000000000cafe0000010000081000",
    "0000000000000000aaaa0000beef00000000000000000000",
    "000000000000000000040003000200010004000300020001",
    "000000000000000000040003000200000004000300020001",
    "000000000000000000040003000200010004000300020001",
    "000000000000000000040003000200000004000300020001",
    "000000000000000000040003000200000004000300020001",
    "000000000000000000040003000200000004000300020001",
    "000000000000000000040003000200000004000300020001",
    "000000000000000000040003000200000004000300020001"
};

sc_biguint<512> str2biguint512(string data) {
    sc_biguint<512> res;
    int length = data.length();
    long long unsigned int val[8];
    for (int i = 7; i >= 0; i--) {
        val[7-i] = stoull(data.substr(16*i, 16), NULL, 16);
    }
    res = (sc_biguint<64>(val[7]), sc_biguint<64>(val[6]), sc_biguint<64>(val[5]), sc_biguint<64>(val[4]), 
        sc_biguint<64>(val[3]), sc_biguint<64>(val[2]), sc_biguint<64>(val[1]), sc_biguint<64>(val[0]));
    return res;
}

sc_biguint<192> str2biguint192(string data) {
    sc_biguint<192> res;
    int length = data.length();
    long long unsigned int val[3];
    for (int i = 2; i >= 0; i--) {
        val[2-i] = stoull(data.substr(16*i, 16), NULL, 16);
    }
    res = (sc_biguint<64>(val[2]), sc_biguint<64>(val[1]), sc_biguint<64>(val[0]));
    return res;
}

SC_MODULE(regfile) {
public:
    sc_in<bool>                      i_clk;
    sc_in<bool>                      i_rst;
    primate_bfu_rdreq_ou::slave<>    bfu_rdreq;
    primate_bfu_rdrsp_ou::master<>   bfu_rdrsp;

    SC_HAS_PROCESS(regfile);
    regfile(sc_module_name name_) {
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    }

    void th_run() {
        primate_bfu_req_t                req;
        
        // Reset
        bfu_rdreq.reset();
        bfu_rdrsp.reset();
        wait();

        while (true) {
            req = bfu_rdreq.read();
            int addr0 = req.addr0;
            int addr1 = req.addr1;
            bfu_rdrsp.write(primate_bfu_rsp_t(str2biguint192(regfile_data[addr0-1]), str2biguint192(regfile_data[addr1-1])));
        }
    }
};

SC_MODULE(source) {
public:
    sc_in<bool>                      i_clk;
    sc_out<bool>                     i_rst;
    
    primate_ctrl_ou::master<>        cmd_out;
    primate_stream_512_4::master<>   pkt_buf_out;
    primate_bfu_ou::slave<>          bfu_in;

    SC_HAS_PROCESS(source);
    source(sc_module_name name_) {
        SC_CTHREAD(th_reset, i_clk.pos());
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);
    }

    void th_reset() {
        i_rst.write(1);
        wait(5);
        i_rst.write(0);
        wait();
    }

    void th_run() {
        sc_uint<NUM_THREADS_LG> tag;
        sc_uint<IP_WIDTH> flag;
        bool early;
        // Reset
        cmd_out.reset();
        bfu_in.reset();
        pkt_buf_out.reset();

        wait();

        // Send stimulus to DUT
        for (int i = 0; i < NUM_PKT; i++) {
            cout << "packet " << i << endl;
            // start_time[i] = sc_time_stamp();

            primate_ctrl_ou::cmd_t cmd(i, 4, 1, HDR_COUNT, 0);
            cmd_out.write(cmd);

            // pkt_buf_out.write(primate_io_payload_t(str2biguint512(pkt_data[0]), i, EMPTY, 0));
            // pkt_buf_out.write(primate_io_payload_t(str2biguint512(pkt_data[1]), i, 0, 0));
            pkt_buf_out.write(primate_io_payload_t(str2biguint512(pkt_data[2]), i, 0, 1));

            bool done = false;
            do {
                done = bfu_in.read(tag, flag, early);
                wait();
            } while (!done);
            cout << "Thread " << tag << ", flag " << flag << ", early " << early << endl;
        }

        // wait(10000);
        // cout << "Hanging simulaiton stopped by TB source thread. Please check DUT module." << endl;
        sc_stop();
    }
};

SC_MODULE(sink) {
public:
    sc_in<bool>                      i_clk;
    sc_in<bool>                      i_rst;

    primate_stream_512_4::slave<>    stream_in;
    std::ofstream outfile;

    SC_HAS_PROCESS(sink);
    sink(sc_module_name name_) {
        outfile.open("output.txt");
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    }

    void th_run() {
        primate_io_payload_t out_data;
        // outfile.open("output.txt");
        // Reset
        stream_in.reset();
        wait();

        // for (int i = 0; i < NUM_PKT; i++) {
            // outfile << "Packet " << i << ":" << endl;
            do {
                out_data = stream_in.read();
                outfile << out_data;
            } while (true);
        // }

        // sc_stop();
    }
};

SC_MODULE(tb) {
public:
    // Module declarations
    source *source_inst;
    sink *sink_inst;
    regfile *regfile_inst;
    outputUnit *outputUnit_inst;

    // local signal declarations
    sc_signal<bool> rst_sig;
    sc_clock        clk_sig;

    primate_stream_512_4::chan<>        dut_to_tb_data;
    primate_ctrl_ou::chan<>             tb_to_dut_ctrl;
    primate_bfu_ou::chan<>              dut_to_tb_ctrl;
    primate_stream_512_4::chan<>        pkt_buf;
    primate_bfu_rdreq_ou::chan<>        bfu_rdreq;
    primate_bfu_rdrsp_ou::chan<>        bfu_rdrsp;

    SC_CTOR(tb) : clk_sig("clk_sig", 4, SC_NS) {
        source_inst = new source("source_inst");
        source_inst->i_clk(clk_sig);
        source_inst->i_rst(rst_sig);
        source_inst->cmd_out(tb_to_dut_ctrl);
        source_inst->pkt_buf_out(pkt_buf);
        source_inst->bfu_in(dut_to_tb_ctrl);

        sink_inst = new sink("sink_inst");
        sink_inst->i_clk(clk_sig);
        sink_inst->i_rst(rst_sig);
        sink_inst->stream_in(dut_to_tb_data);

        regfile_inst = new regfile("regfile_inst");
        regfile_inst->i_clk(clk_sig);
        regfile_inst->i_rst(rst_sig);
        regfile_inst->bfu_rdreq(bfu_rdreq);
        regfile_inst->bfu_rdrsp(bfu_rdrsp);

        outputUnit_inst = new outputUnit("outputUnit_inst");
        outputUnit_inst->i_clk(clk_sig);
        outputUnit_inst->i_rst(rst_sig);
        outputUnit_inst->stream_out(dut_to_tb_data);
        outputUnit_inst->cmd_in(tb_to_dut_ctrl);
        outputUnit_inst->bfu_out(dut_to_tb_ctrl);
        outputUnit_inst->bfu_rdreq(bfu_rdreq);
        outputUnit_inst->bfu_rdrsp(bfu_rdrsp);
        outputUnit_inst->pkt_buf_in(pkt_buf);
    }

    ~tb() {
        delete source_inst;
        delete sink_inst;
        delete regfile_inst;
        delete outputUnit_inst;
    }
};

int sc_main(int argc, char* argv[])
{
    tb *top = new tb("my_tb");
    sc_report_handler::set_actions(SC_ERROR, SC_DISPLAY);
    sc_start();
    if(sc_report_handler::get_count(SC_ERROR) > 0) {
        cout << "Simulation FAILED" << endl;
        return -1;
    } else {
        cout << "Simulation PASSED" << endl;
    }
    return 0;
}