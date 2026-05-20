#!/bin/bash

run_it() {
    # export GLIBCXX_TUNABLES="glibcxx.eh_pool.obj_count=100;glibcxx.eh_pool.obj_size=128"
    west build -t run
}


run_it 2>&1 | tee run.log

