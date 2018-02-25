#! /bin/bash
./siege -c 20000 -r 200  --log=./x.log   'http://192.168.1.106:25000/im/hello POST < post_data.json'
