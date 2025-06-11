include=-I$(HOME)/AIMSLab/motive-stream/dependencies/NatNet/include
libpath=-L$(HOME)/AIMSLab/motive-stream/dependencies/NatNet/lib

libs=-lNatNet

all:build/SampleClient

build/SampleClient: 
	g++ SampleClient.cpp $(include) $(libpath) $(libs) -o build/SampleClient

.PHONY: clean
clean:
	@rm -f ./build/SampleClient
