all: uart_tx uart_rx

uart_tx:
	$(MAKE) -C uart_TX/ EXEC_DIR=$(EXEC_DIR)

uart_rx:
	$(MAKE) -C uart_RX/ EXEC_DIR=$(EXEC_DIR)


clean:
	$(MAKE) -C uart_TX/ clean EXEC_DIR=$(EXEC_DIR)
	$(MAKE) -C uart_RX/ clean EXEC_DIR=$(EXEC_DIR)
