connect -url tcp:127.0.0.1:3121
source C:/Datos/Pablo/IOT/1semestre/Sistemas_digitales_embebidos/Practicas/proy/zynq_wrapper_hw_platform1/ps7_init.tcl
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279759359A"} -index 0
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent Zybo 210279759359A" && level==0} -index 1
fpga -file C:/Datos/Pablo/IOT/1semestre/Sistemas_digitales_embebidos/Practicas/proy/zynq_wrapper_hw_platform1/design_1_wrapper.bit
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279759359A"} -index 0
loadhw -hw C:/Datos/Pablo/IOT/1semestre/Sistemas_digitales_embebidos/Practicas/proy/zynq_wrapper_hw_platform1/system.hdf -mem-ranges [list {0x40000000 0xbfffffff}]
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279759359A"} -index 0
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo 210279759359A"} -index 0
dow C:/Datos/Pablo/IOT/1semestre/Sistemas_digitales_embebidos/Practicas/proy/blinky1/Debug/blinky1.elf
configparams force-mem-access 0
bpadd -addr &main
