CXXFLAGS = -g -Wall -Werror -pthread -lssl -lcrypto

TARGETS = CentralServer mempool Minero MPserver Wallet

all: $(TARGETS)

CentralServer: CentralServer.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
	rm -f CentralServer.o
mempool: mempool.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
	rm -f mempool.o

Minero: Minero.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
	rm -f Minero.o
MPserver: MPserver.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
	rm -f MPserver.o
Wallet: Wallet.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
	rm -f Wallet.o
	
%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -f ${TARGETS}  core* *.o
