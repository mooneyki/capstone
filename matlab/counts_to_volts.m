function [ volts ] = counts_to_volts( counts )
%converts ADC counts to voltage
%   can be modified for any ADC based on the number of bits and full scale
%   voltage
adc_fs = 3.3;
bits = 12;
max_counts = (2^bits) - 1; 
volts = ( counts / max_counts ) * adc_fs;
end

