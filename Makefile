CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -pedantic
BACKEND_SRC = backend/src/engine.cpp backend/src/generated_data.cpp

.PHONY: all backend frontend simulate clean

all: backend frontend

backend: backend/build/french_rev_server backend/build/french_rev_sim

backend/build:
	mkdir -p backend/build

backend/build/french_rev_server: backend/src/main.cpp $(BACKEND_SRC) backend/src/*.hpp | backend/build
	$(CXX) $(CXXFLAGS) -Ibackend/src backend/src/main.cpp $(BACKEND_SRC) -o $@

backend/build/french_rev_sim: backend/src/simulate.cpp $(BACKEND_SRC) backend/src/*.hpp | backend/build
	$(CXX) $(CXXFLAGS) -Ibackend/src backend/src/simulate.cpp $(BACKEND_SRC) -o $@

frontend:
	npm --prefix frontend install
	npm --prefix frontend run build

simulate: backend/build/french_rev_sim
	./backend/build/french_rev_sim

clean:
	rm -rf backend/build frontend/dist
