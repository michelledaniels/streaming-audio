SHELL=/bin/bash

all: 
	$(MAKE) -C src/sam
	$(MAKE) -C src/client
	$(MAKE) -C src/render
	$(MAKE) -C src/sam/test
	$(MAKE) -C src/client/examples/saminput
	$(MAKE) -C src/client/examples/samugen 
	$(MAKE) -C src/client/examples/samugen-gui 
	$(MAKE) -C src/render/examples/testrenderer 

clean:
	$(MAKE) -C src/sam clean
	$(MAKE) -C src/client clean
	$(MAKE) -C src/render clean
	$(MAKE) -C src/sam/test clean
	$(MAKE) -C src/client/examples/saminput clean
	$(MAKE) -C src/client/examples/samugen clean
	$(MAKE) -C src/client/examples/samugen-gui clean
	$(MAKE) -C src/render/examples/testrenderer clean

install:
	$(MAKE) -C src/sam install
	$(MAKE) -C src/client install
	$(MAKE) -C src/render install
	$(MAKE) -C src/sam/test install
	$(MAKE) -C src/client/examples/saminput install
	$(MAKE) -C src/client/examples/samugen install
	$(MAKE) -C src/client/examples/samugen-gui install
	$(MAKE) -C src/render/examples/testrenderer install
