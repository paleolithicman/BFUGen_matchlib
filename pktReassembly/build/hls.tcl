source global_hls.tcl

# Read Design Files
solution file add ../pktReassembly.cpp
solution file add ../tb.cpp -exclude true
# solution file add $env(HLSTOP)/rdma_core/csrc/ib_def.cpp
# solution file add $env(HLSTOP)/rdma_core/csrc/common.cpp
# solution design set inputUnit -top
solution design set pktReassembly -top
options set /Output/SubBlockNamePrefix inputUnit_
options set /Input/CompilerFlags {-DCONNECTIONS_NAMING_ORIGINAL -DMB_ENABLE_IN_PEEK}
go compile

solution library add mgc_Xilinx-VIRTEX-uplus-2L_beh -- -rtlsyntool Vivado -manufacturer Xilinx -family VIRTEX-uplus -speed -2L -part xcu250-figd2104-2L-e
solution library add Xilinx_RAMS
go libraries

directive set -CLOCKS {i_clk {-CLOCK_PERIOD 10 -CLOCK_HIGH_TIME 5 -CLOCK_OFFSET 0.000000 -CLOCK_UNCERTAINTY 0.0}}
go assembly
go extract
flow run /SCVerify/launch_make ./scverify/Verify_concat_sim_rtl_v_msim.mk {} SIMTOOL=msim simgui
