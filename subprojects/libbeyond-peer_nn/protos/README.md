# Generate C++ Code

run the protocol buffer compiler protoc, specifying the source directory (where your application's source code lives – the current directory is used if you don't provide a value), the destination directory (where you want the generated code to go; often the same as $SRC_DIR), and the path to your .proto. In this case, you would invoke:
``` bash
$ protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/addressbook.proto
```
https://developers.google.com/protocol-buffers/docs/reference/cpp-generated

## example
``` bash
$ protoc --cpp_out ./ peer_nn.proto
$ protoc --grpc_out ./ --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin peer_nn.proto
```
