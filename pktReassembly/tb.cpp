#include <systemc.h>
#include "common.h"
#include "pktReassembly.h"
#include <map>
#define NUM_PKT 10
using namespace std;

static map<sc_biguint<96>, fce_t> flow_table;
static vector<dymem_t> mem(512);
static bool mem_valid[512];

sc_biguint<272> str2biguint(string data) {
    sc_biguint<272> res;
    int length = data.length();
    long long unsigned int val[5];
    for (int i = 3; i >= 0; i--) {
        val[3-i] = stoull(data.substr(16*i, 16), NULL, 16);
    }
    res = (0, sc_biguint<64>(val[3]), sc_biguint<64>(val[2]), sc_biguint<64>(val[1]), sc_biguint<64>(val[0]));
    return res;
}

SC_MODULE(lock) {
public:
    sc_in<bool>                      i_clk;
    sc_in<bool>                      i_rst;

    bfu_in::slave<>                  CCS_INIT_S1(lock_in);
    bfu_out::master<>                CCS_INIT_S1(lock_out);

    SC_HAS_PROCESS(lock);
    lock(sc_module_name name_) {
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    }

    void th_run() {
        bfu_in_pl_t cmd;
        lock_in.reset();
        lock_out.reset();
        wait();

        while(true) {
            cmd = lock_in.read();
            lock_out.write(bfu_out_pl_t(cmd.tag, 0, 0));
        }
    }
};

SC_MODULE(flow_table_read) {
public:
    sc_in<bool>                      i_clk;
    sc_in<bool>                      i_rst;

    bfu_in::slave<>                  CCS_INIT_S1(flow_table_read_in);
    bfu518_out::master<>             CCS_INIT_S1(flow_table_read_out);

    SC_HAS_PROCESS(flow_table_read);
    flow_table_read(sc_module_name name_) {
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    }

    void th_run() {
        bfu_in_pl_t cmd;
        meta_t input;
        fce_t fte;
        flow_table_read_in.reset();
        flow_table_read_out.reset();
        wait();

        while(true) {
            cmd = flow_table_read_in.read();
            input.set(cmd.bits);
            sc_biguint<96> key = input.tuple;
            if (flow_table.find(key) != flow_table.end()) {
                fte = flow_table[key];
            } else {
                fte.ch0_bit_map = 0;
            }
            flow_table_read_out.write(bfu518_out_pl_t(cmd.tag, 0, (fte.to_uint(), input.to_uint())));
        }
    }
};

SC_MODULE(flow_table_write) {
public:
    sc_in<bool>                      i_clk;
    sc_in<bool>                      i_rst;

    bfu_in::slave<>                  CCS_INIT_S1(flow_table_write_in);
    // bfu_out::master<>                CCS_INIT_S1(flow_table_write_out);

    SC_HAS_PROCESS(flow_table_write);
    flow_table_write(sc_module_name name_) {
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    }

    void th_run() {
        bfu_in_pl_t cmd;
        flow_table_write_in.reset();
        // flow_table_write_out.reset();
        wait();
        
        while (true) {
            cmd = flow_table_write_in.read();
            if (cmd.opcode == 1) {
                // insert
                meta_t input;
                input.set(cmd.bits);
                if ((input.tcp_flags & (1 << TCP_FIN) | (input.tcp_flags & (1 << TCP_RST))) == 0) {
                    sc_biguint<96> key = input.tuple;
                    fce_t tmp;
                    tmp.tuple = input.tuple;
                    if ((input.tcp_flags & (1 << TCP_SYN)) != 0) {
                        tmp.seq = input.seq + 1;
                    } else {
                        tmp.seq = input.seq + input.len;
                    }
                    tmp.pointer = 0;
                    tmp.slow_cnt = 0;
                    tmp.pointer2 = 0;
                    tmp.ch0_bit_map = 1;
                    flow_table[key] = tmp;
                }
            } else if (cmd.opcode == 2) {
                fce_t fte;
                fte.set(cmd.bits);
                sc_biguint<96> key = fte.tuple;
                flow_table[key] = fte;
            } else if (cmd.opcode == 3) {
                fce_t fte;
                fte.set(cmd.bits);
                sc_biguint<96> key = fte.tuple;
                flow_table.erase(key);
            }
            // flow_table_write_out.write(bfu_out_pl_t(cmd.tag, 0, 0));
        }
    }
};

SC_MODULE(source) {
public:
    sc_in<bool>                      i_clk;
    sc_out<bool>                     i_rst;

    primate_stream_272_4::master<>   CCS_INIT_S1(stream_out);

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

        infile.open("/home/marui/crossroad/HLS/matchlib_kit/primate/pktReassembly/input.txt");
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

            primate_ctrl_iu::cmd_t cmd(i, 0, 1, 0, 0);
            cmd_out.write(cmd);

// #pragma hls_pipeline_init_interval 1
            do {
                infile >> last >> empty >> indata;
                primate_stream_272_4::payload_t payload(str2biguint(indata), i, empty, last);
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
    sc_in<bool>                      i_rst;
    primate_bfu_iu::slave<>          CCS_INIT_S1(bfu_in);
    primate_stream_272_4::slave<>    CCS_INIT_S1(stream_in);

    std::ofstream outfile;

    SC_HAS_PROCESS(sink);
    sink(sc_module_name name_) {
        outfile.open("output.txt");
        SC_CTHREAD(th_run, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    }

    void th_run() {
        map<int, int> reg2idx{{1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 5}, {7, 6}, {22, 7}};
        int idx2reg[8] = {1, 2, 3, 4, 5, 6, 7, 22};
        sc_biguint<REG_WIDTH> regs[8];

        // Extract clock period
        sc_clock *clk_p = dynamic_cast<sc_clock*>(i_clk.get_interface());
        auto clock_period = clk_p->period();

        // outfile.open("/home/marui/crossroad/HLS/BFUGen/pktReassembly/output.txt");

        // Initialize port
        bfu_in.reset();
        stream_in.reset();
        primate_stream_272_4::payload_t payload;
        primate_bfu_iu::payload_t iu_out;

        wait();

        double total_cycles = 0;

        sc_time start_time = sc_time_stamp();

        // Read output coming from DUT
        for (int i = 0; i < NUM_PKT; i++) {
            outfile << "Thread " << i << ":" << endl;
            bool done = false;
            bool output_valid = false;
// #pragma hls_pipeline_init_interval 1
            do {
                // if (out_wen[0].read() || out_wen[1].read()) {
                //     cout << sc_time_stamp() << ": waddr0 " << out_addr[0].read() << ", wadd1 " << out_addr[1].read() << endl;
                //     cout << sc_time_stamp() << ": wdata0 " << hex << out_data[0].read() << ", wdata1 " << out_data[1].read() << dec << endl;
                // }
                if (stream_in.nb_read(payload)) {
                    output_valid = true;
                }
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
                wait();
            } while (!done);
            // end_time[i] = sc_time_stamp();
            // total_cycles += (end_time[i] - start_time[i]) / clock_period;

            // Print outputs
            for (int j = 0; j < 8; j++) {
                outfile << "REG " << idx2reg[j] << ": " << hex << regs[j] << dec << endl;
            }
            if (output_valid) {
                outfile << payload;
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
    lock *lock_inst;
    flow_table_read *flow_table_read_inst;
    flow_table_write *flow_table_write_inst;
    pktReassembly *pktReassembly_inst;

    // local signal declarations
    sc_signal<bool> rst_sig;
    sc_clock        clk_sig;

    primate_stream_272_4::chan<>        CCS_INIT_S1(tb_to_dut_data);
    primate_stream_272_4::chan<>        CCS_INIT_S1(dut_to_tb_data);
    primate_ctrl_iu::chan<>             CCS_INIT_S1(tb_to_dut_ctrl);
    primate_bfu_iu::chan<>              CCS_INIT_S1(dut_to_tb_ctrl);

    // bfu_in::chan<>                      CCS_INIT_S1(dut_to_lock);
    // bfu_out::chan<>                     CCS_INIT_S1(lock_to_dut);
    bfu_in::chan<>                      CCS_INIT_S1(dut_to_ft_read);
    bfu518_out::chan<>                  CCS_INIT_S1(ft_read_to_dut);
    bfu_in::chan<>                      CCS_INIT_S1(dut_to_ft_write);
    // bfu_out::chan<>                     CCS_INIT_S1(ft_write_to_dut);

    SC_CTOR(tb) : clk_sig("clk_sig", 4, SC_NS) {
        source_inst = new source("source_inst");
        source_inst->i_clk(clk_sig);
        source_inst->i_rst(rst_sig);
        source_inst->stream_out(tb_to_dut_data);
        source_inst->cmd_out(tb_to_dut_ctrl);

        sink_inst = new sink("sink_inst");
        sink_inst->i_clk(clk_sig);
        sink_inst->i_rst(rst_sig);
        sink_inst->bfu_in(dut_to_tb_ctrl);
        sink_inst->stream_in(dut_to_tb_data);

        // lock_inst = new lock("lock_inst");
        // lock_inst->i_clk(clk_sig);
        // lock_inst->i_rst(rst_sig);
        // lock_inst->lock_in(dut_to_lock);
        // lock_inst->lock_out(lock_to_dut);

        flow_table_read_inst = new flow_table_read("flow_table_read_inst");
        flow_table_read_inst->i_clk(clk_sig);
        flow_table_read_inst->i_rst(rst_sig);
        flow_table_read_inst->flow_table_read_in(dut_to_ft_read);
        flow_table_read_inst->flow_table_read_out(ft_read_to_dut);

        flow_table_write_inst = new flow_table_write("flow_table_write_inst");
        flow_table_write_inst->i_clk(clk_sig);
        flow_table_write_inst->i_rst(rst_sig);
        flow_table_write_inst->flow_table_write_in(dut_to_ft_write);
        // flow_table_write_inst->flow_table_write_out(ft_write_to_dut);

        pktReassembly_inst = new pktReassembly("pktReassembly_inst");
        pktReassembly_inst->i_clk(clk_sig);
        pktReassembly_inst->i_rst(rst_sig);
        pktReassembly_inst->stream_in(tb_to_dut_data);
        pktReassembly_inst->stream_out(dut_to_tb_data);
        pktReassembly_inst->cmd_in(tb_to_dut_ctrl);
        pktReassembly_inst->bfu_out(dut_to_tb_ctrl);
        pktReassembly_inst->flow_table_read_req(dut_to_ft_read);
        pktReassembly_inst->flow_table_read_rsp(ft_read_to_dut);
        pktReassembly_inst->flow_table_write_req(dut_to_ft_write);
        // pktReassembly_inst->flow_table_write_rsp(ft_write_to_dut);
        // pktReassembly_inst->lock_req(dut_to_lock);
        // pktReassembly_inst->lock_rsp(lock_to_dut);
    }

    ~tb() {
        delete source_inst;
        delete sink_inst;
        delete lock_inst;
        delete flow_table_read_inst;
        delete flow_table_write_inst;
        delete pktReassembly_inst;
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
