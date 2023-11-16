# uart_data_display
Displays incoming data from UART channel relating to accelerometer peripheral.

# How to use
This program uses the Win32 API to read from the COM Ports. You will need to have an UART connection sending data to the COM Port with 8N1, 115200 baud rate.
It expects data in the format (accelerometer_x, accelerometer_y, pos_x, pos_y), using commas as delimiters.
It will likely run very slowly if the UART data does not arrive quickly enough, but it will also fail if the data arrives too quickly. I had good results giving it a 2 ms delay per transmission.
