FLAG=-g -std=c++11 -I ~/third_part/include -L ~/third_part/lib\
		 -lprotobuf -lglog -lgflags -lpthread

.PHONY:all
all:index_builder index_dump

index_builder:index_builder.cc libindex.a
	g++ $^ -o $@ $(FLAG)
	cp -f $@ ../bin/

index_dump:index_dump.cc libindex.a
	g++ $^ -o $@ $(FLAG)
	cp -f $@ ../bin/

libindex.a:index.cc index.pb.cc
	g++ -c index.cc -o index.o $(FLAG)
	g++ -c index.pb.cc -o index.pb.o $(FLAG)
	ar -rc libindex.a *.o

index.pb.cc:index.proto
	~/third_part/bin/protoc index.proto --cpp_out=.

.PHONY:clean
clean:
	rm index_builder index_dump *.o *.pb.*
