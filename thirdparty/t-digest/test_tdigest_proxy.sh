#!/bin/bash

LD_LIBRARY_PATH+=:/lib/jvm/java/jre/lib/amd64:/lib/jvm/java/jre/lib/amd64/server:/lib/jvm/java/jre/lib/amd64/client
export LD_LIBRARY_PATH

./test_tdigest_proxy
