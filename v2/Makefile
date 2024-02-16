seg_name := $(shell grep "Segment" default.ini | cut -d '=' -f 2)
dem_name := $(shell grep "Dem" default.ini | cut -d '=' -f 2)

.PHONY: all_one_seg 
all_one_seg : graphs/images/${seg_name}/%.png

graphs/images/${seg_name}/%.png &: graphs/dat/${seg_name}.dat
	bash commands/fasrawc_generetate_graph_comparaison.sh ${seg_name} ${dem_name}
graphs/dat/${seg_name}.dat: seg
	mv ${seg_name}.dat graphs/dat
.PHONY: seg 
seg: logs/${seg_name}.log
	bash commands/count_fastwarc_result.sh logs/${seg_name}.log ${seg_name} 
logs/${seg_name}.log: default.ini 
	python3 src/main.py 
     

