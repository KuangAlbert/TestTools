----------------------------------------
test tools v0.1
2016/11/05
----------------------------------------

.............................................................................................................................................................
file:

├── Archive
│   ├── i2ctools
│   │   ├── i2c_tool.rar
│   │   └── i2c-tools-3.rar
│   ├── spi_test_0924.tar.gz
│   ├── testing-tuner.tar.gz
│   └── uart_test_G5.rar
└── test_program_ds03h_norflash
    └── usr
        ├── bin
        │   ├── persistservice
        │   └── test.persist
        └── lib
            ├── libbasic.so.1.0.0
            ├── libpersist.so.1.0.0
            └── libservicefw.so.1.0.0
i2ctools:
	It can build i2cdetect tools and which has a good translation can be used directly.
	Use as follows:
		./i2cdetect –l			:checkout bus
		./i2cdetect -r -y 1		:Detection of i2c mount
		./i2cdump -f -y 1 0x20  :View the value of the register

spitest_0924.tar.gz:
	Raw SPI test program compressed package

testing-tuner.tar.gz

uart_test_G5.rar

test_program_ds03h_norflash:test norflash speed

..............................................................................................................................................................
program:

.
├── spi_test
├── testing-tuner
└── uart_89501_tool
    ├── uart_RX
	    └── uart_TX

Use as follows:
1 spi_test:test example: ./spi_test 02 02 01 0A 00 00 01 02 03 04 05 06 07 08 09

2 test tuner: test_tuner parameter1 parameter2 parameter3
	parameter1:set output
		0:Analog output
		1:Digital output

	parameter2:set input
		wave:sin wave
		9440:radio frequency

	parameter3:set master/slave
		master:
		slave:

3 uart_89501_tool
	uart_89501 switch\ ports
	uart_89501_rx &

...............................................................................................................................................................
script:
.
├── ds03h.sh
├── gotolinux.sh
└── make.sh

	make.sh:for ds03h download
	./make.sh: download all
	./make.sh 0: download uboot and kernel
	./make.sh 1:download kernel
	./make.sh 2:download rootfs

.................................................................................................................................................................
