dataset=$1

/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=build --benchmark_type=${dataset} --build_type=plain --thread_n=8 2>&1 | tee build_${dataset}_plain_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=build --benchmark_type=${dataset} --build_type=lvq --thread_n=8 2>&1 | tee build_${dataset}_lvq_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=build --benchmark_type=${dataset} --build_type=crabitq --thread_n=8 2>&1 | tee build_${dataset}_crabitq_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=build --benchmark_type=${dataset} --build_type=lsg --thread_n=8 2>&1 | tee build_${dataset}_lsg_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=build --benchmark_type=${dataset} --build_type=lvqlsg --thread_n=8 2>&1 | tee build_${dataset}_lvqlsg_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=build --benchmark_type=${dataset} --build_type=crabitqlsg --thread_n=8 2>&1 | tee build_${dataset}_crabitqlsg_16_200.log

/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=query --benchmark_type=${dataset} --build_type=plain --thread_n=8 --topk=10 2>&1 | tee query_${dataset}_plain_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=query --benchmark_type=${dataset} --build_type=lvq --thread_n=8 --topk=10 2>&1 | tee query_${dataset}_lvq_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=query --benchmark_type=${dataset} --build_type=crabitq --thread_n=8 --topk=10 2>&1 | tee query_${dataset}_crabitq_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=query --benchmark_type=${dataset} --build_type=lsg --thread_n=8 --topk=10 2>&1 | tee query_${dataset}_lsg_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=query --benchmark_type=${dataset} --build_type=lvqlsg --thread_n=8 --topk=10 2>&1 | tee query_${dataset}_lvqlsg_16_200.log
/usr/bin/time -v ./cmake-build-release/benchmark/local_infinity/hnsw_benchmark --mode=query --benchmark_type=${dataset} --build_type=crabitqlsg --thread_n=8 --topk=10 2>&1 | tee query_${dataset}_crabitqlsg_16_200.log
