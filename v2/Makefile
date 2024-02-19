seg_name := $(shell grep "Segment" default.ini | cut -d '=' -f 2)
dem_name := $(shell grep "Dem" default.ini | cut -d '=' -f 2)
num_segs :=  $(shell grep "NumberOfSegs" default.ini | cut -d '=' -f 2)
all_dats_file_name := $(shell date +"all_%Y_%m_%d:%H:%m:%S")

.PHONY: all 
all: dat_all 
	bash commands/fasrawc_generetate_graph_comparaison.sh ${all_dats_file_name} ${dem_name}
#graphs/dat/${all_dats_file_name}.dat: logs/${all_dats_file_name}.log
.PHONY: dat_all
dat_all: log_all 
	bash commands/count_fastwarc_result.sh logs/${all_dats_file_name}.log ${all_dats_file_name}; mv ${all_dats_file_name}.dat graphs/dat
.PHONY: log_all
log_all:
	for i in $$(seq 0 ${num_segs}); do echo "executing script for seg : $$i\n"; seg="0000$$i"; corr_seg=$$(echo -n $$seg | tail -c 5) ;  sed -i.bak "s/\(Segment=\).*/\1$${corr_seg}/g" default.ini ; seg_tmp=$$(grep "Segment" default.ini | cut -d '=' -f 2); $(MAKE) all_one_seg; if [ $$i -eq 0 ] ; then cat logs/$${seg_tmp}.log > logs/${all_dats_file_name}.log ;else tail -n +2 logs/$${seg_tmp}.log >> logs/${all_dats_file_name}.log; fi ; done  

.PHONY: all_one_seg 
all_one_seg : images_one_seg 
.PHONY:images_one_seg
images_one_seg:  graphs/dat/${seg_name}.dat
	bash commands/fasrawc_generetate_graph_comparaison.sh ${seg_name} ${dem_name}
graphs/dat/${seg_name}.dat: seg
	mv ${seg_name}.dat graphs/dat
.PHONY: seg 
seg: logs/${seg_name}.log
	bash commands/count_fastwarc_result.sh logs/${seg_name}.log ${seg_name} 
logs/${seg_name}.log: default.ini 
	python3 src/main.py 

.PHONY: clean
clean:     
	rm logs/* 2>/dev/null
	rm -r graphs/images/* 2>/dev/null
	rm graphs/dat/* 2>/dev/null
