 #../../l3oss/bin/protoc -I=.. --cpp_out=.  ../hello_test.proto
../l3oss/bin/thrift -nowarn -o . --gen cpp  ./src/hello_test.thrift
