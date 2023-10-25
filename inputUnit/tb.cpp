#include <systemc.h>
#include "common.h"
#include "inputUnit.h"
#include <map>
#define NUM_PKT 2
using namespace std;

sc_biguint<512> str2biguint(string data) {
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

SC_MODULE(source) {
public:
    sc_in<bool>                      i_clk;
    sc_out<bool>                     i_rst;

    primate_stream_512_4::master<>   CCS_INIT_S1(stream_out);

    primate_ctrl_iu::master<>        CCS_INIT_S1(cmd_out);
    std::ifstream infile;

    SC_HAS_PROCESS(source);
    source(sc_module_name name_) {
        SC_CTHREAD(th_run, i_clk.pos());
    }

    void th_run() {
        string indata;
        int    empty;
        bool   last;

        infile.open("/home/marui/crossroad/HLS/matchlib_kit/primate/inputUnit/input.txt");
        // infile.open("input.txt");

        // Reset
        stream_out.reset();
        cmd_out.reset();
        i_rst.write(1);
        wait();
        i_rst.write(0);
        wait();

        // Send stimulus to DUT
        for (int i = 0; i < NUM_PKT; i++) {
            cout << "packet " << i << endl;
            // start_time[i] = sc_time_stamp();

            primate_ctrl_iu::cmd_t cmd(i, 4, 1, 0, 0);
            cmd_out.write(cmd);

// #pragma hls_pipeline_init_interval 1
            do {
                infile >> last >> empty >> indata;
                primate_stream_512_4::payload_t payload(str2biguint(indata), i, empty, last);
                stream_out.write(payload);
            } while (!last);
        }

        wait(10000);
        cout << "Hanging simulaiton stopped by TB source thread. Please check DUT module." << endl;
        sc_stop();
    }

};

SC_MODULE(sink) {
public:
    sc_in<bool>                      i_clk;
    primate_bfu_iu::slave<>          CCS_INIT_S1(bfu_in);

    primate_stream_512_4::slave<>    CCS_INIT_S1(pkt_buf_in);

    std::ofstream outfile;

    SC_HAS_PROCESS(sink);
    sink(sc_module_name name_) {
        outfile.open("output.txt");
        SC_CTHREAD(th_run, i_clk.pos());
    }

    void th_run() {
        map<int, int> reg2idx{{1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 5}, {7, 6}, {22, 7}};
        int idx2reg[8] = {1, 2, 3, 4, 5, 6, 7, 22};
        sc_biguint<REG_WIDTH> regs[8];
        primate_stream_512_4::payload_t pkt_buf[4];

        // Extract clock period
        sc_clock *clk_p = dynamic_cast<sc_clock*>(i_clk.get_interface());
        auto clock_period = clk_p->period();

        // outfile.open("/home/marui/crossroad/HLS/BFUGen/inputUnit/output.txt");

        // Initialize port
        bfu_in.reset();
        pkt_buf_in.reset();
        primate_stream_512_4::payload_t payload;
        primate_bfu_iu::payload_t iu_out;

        double total_cycles = 0;

        sc_time start_time = sc_time_stamp();

        // Read output coming from DUT
        for (int i = 0; i < NUM_PKT; i++) {
            outfile << "Thread " << i << ":" << endl;
            int num_pkt_buf = 0;
            bool done = false;
// #pragma hls_pipeline_init_interval 1
            do {
                // if (out_wen[0].read() || out_wen[1].read()) {
                //     cout << sc_time_stamp() << ": waddr0 " << out_addr[0].read() << ", wadd1 " << out_addr[1].read() << endl;
                //     cout << sc_time_stamp() << ": wdata0 " << hex << out_data[0].read() << ", wdata1 " << out_data[1].read() << dec << endl;
                // }
                wait();
                if (bfu_in.nb_read(iu_out)) {
                    if (iu_out.wen0) {
                        int regid = iu_out.addr0;
                        regs[reg2idx[regid]] = iu_out.data0;
                    }
                    if (iu_out.wen1) {
                        int regid = iu_out.addr1;
                        regs[reg2idx[regid]] = iu_out.data1;
                    }
                    done = iu_out.done;
                }
                if (pkt_buf_in.nb_read(payload)) {
                    pkt_buf[num_pkt_buf] = payload;
                    num_pkt_buf++;
                }
            } while (!done);
            // end_time[i] = sc_time_stamp();
            // total_cycles += (end_time[i] - start_time[i]) / clock_period;

            // Print outputs
            for (int j = 0; j < 8; j++) {
                outfile << "REG " << idx2reg[j] << ": " << hex << regs[j] << dec << endl;
            }
            for (int j = 0; j < num_pkt_buf; j++) {
                outfile << pkt_buf[j];
            }
        }

        sc_time end_time = sc_time_stamp();
        total_cycles = (end_time - start_time) / clock_period;

        // Print latency
        // double total_throughput = (start_time[NUM_PKT-1] - start_time[0]) / clock_period;
        printf("Average lantency is %g cycles.\n", (double)(total_cycles));
        // printf("Average throughput is %g cycles per input.\n", (double)(total_throughput/64));

        // End Simulation
        sc_stop();
    }
};

SC_MODULE(tb) {
public:
    // Module declarations
    source *source_inst;
    sink *sink_inst;
    inputUnit *inputUnit_inst;

    // local signal declarations
    sc_signal<bool> rst_sig;
    sc_clock        clk_sig;

    primate_stream_512_4::chan<>        CCS_INIT_S1(tb_to_dut_data);

    primate_ctrl_iu::chan<>             CCS_INIT_S1(tb_to_dut_ctrl);

    primate_bfu_iu::chan<>              CCS_INIT_S1(dut_to_tb_data);

    primate_stream_512_4::chan<>        CCS_INIT_S1(pkt_buf);

    SC_CTOR(tb) : clk_sig("clk_sig", 4, SC_NS) {
        source_inst = new source("source_inst");
        source_inst->i_clk(clk_sig);
        source_inst->i_rst(rst_sig);
        source_inst->stream_out(tb_to_dut_data);
        source_inst->cmd_out(tb_to_dut_ctrl);

        sink_inst = new sink("sink_inst");
        sink_inst->i_clk(clk_sig);
        sink_inst->bfu_in(dut_to_tb_data);
        sink_inst->pkt_buf_in(pkt_buf);

        inputUnit_inst = new inputUnit("inputUnit_inst");
        inputUnit_inst->i_clk(clk_sig);
        inputUnit_inst->i_rst(rst_sig);
        inputUnit_inst->stream_in(tb_to_dut_data);
        inputUnit_inst->cmd_in(tb_to_dut_ctrl);
        inputUnit_inst->bfu_out(dut_to_tb_data);
        inputUnit_inst->pkt_buf_out(pkt_buf);
    }

    ~tb() {
        delete source_inst;
        delete sink_inst;
        delete inputUnit_inst;
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
