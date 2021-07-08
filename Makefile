all:
#	$(MAKE) -C Platform all
	$(MAKE) -C ExtLibs all
	$(MAKE) -C Modules all
	$(MAKE) -C Adp all
	$(MAKE) -C App all
#	$(MAKE) -C ModulesTest all
	$(MAKE) -C Src all
clean:
	$(MAKE) -C ExtLibs clean
	$(MAKE) -C Modules clean
	$(MAKE) -C Adp clean
	$(MAKE) -C App clean
#	$(MAKE) -C ModulesTest clean
	$(MAKE) -C Src clean	
