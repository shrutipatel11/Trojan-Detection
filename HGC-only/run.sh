taskgraph="graph1"
n_hgc=3
ip_hgc_scheduler="127.0.0.1"
ip_hgc_slaves=("127.0.0.1" "127.0.0.1" "127.0.0.1")
port_hgc_slaves=(10001 10002 10003 10004 10005)
task_hgc_slaves=(30001 30002 30003 30004 30005)
hgc_core=(1 2 3)
output="Report_hgc_only"

taskset -c 0 python hgc_scheduler.py $taskgraph $n_hgc "${ip_hgc_slaves[@]:0:$n_hgc}" "${port_hgc_slaves[@]:0:$n_hgc}" $output&
sleep 2

for (( i=0; i<$n_hgc; i++ ))
do
  taskset -c "${hgc_core[i]}" python hgc_slave.py $ip_hgc_scheduler "${port_hgc_slaves[i]}" "${task_hgc_slaves[$i]}" &
done
