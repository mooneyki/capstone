%scales, offsets
torque_scale = 1;
torque_offset = 0;

temp_scale = 1;
temp_offset = 0;

belt_temp_scale = 1;
belt_temp_offset = 0;

i_brake_scale = 1;
i_brake_offset = 0;

load_cell_scale = 1;
load_cell_offset = 0;

tps_scale = 1;
tps_offset = 0;

%parse data from files
%each quantity has its own column
filename = 'filename.csv';
dp = csvread(filename); %contains all columns 
num_rows = size(dp,1); %depends on test
num_cols = size(dp,2); %should be 12

%populate data points
prim_rpm = zeros(num_rows,1);
sec_rpm = zeros(num_rows,1);        
torque = zeros(num_rows,1);
temp3 = zeros(num_rows,1);       
belt_temp = zeros(num_rows,1);      
temp2 = zeros(num_rows,1);
i_brake = zeros(num_rows,1);     
temp1 = zeros(num_rows,1);          
load_cell = zeros(num_rows,1); 
tps = zeros(num_rows,1);         
i_sp = zeros(num_rows,1);           
tps_sp = zeros(num_rows,1);

for i = 1:num_rows
    prim_rpm(i,1) = dp(i,1);
    sec_rpm(i,1) = dp(i,2);
    torque(i,1) = dp(i,3);
    temp3(i,1) = dp(i,4);
    belt_temp(i,1) = dp(i,5);
    temp2(i,1) = dp(i,6);
    i_brake(i,1) = dp(i,7);
    temp1(i,1) = dp(i,8);
    load_cell(i,1) = dp(i,9);
    tps(i,1) = dp(i,10);
    i_sp(i,1) = dp(i,11);
    tps_sp(i,1) = dp(i,12);
end

%analog value conversions
for i = 1:num_rows
    torque(i,1) = ( counts_to_volts ( torque(i,1) ) ) * torque_scale + torque_offset ;
    temp3(i,1) = ( counts_to_volts ( temp3(i,1) ) ) * temp_scale + temp_offset; 
    belt_temp(i,1) = ( counts_to_volts ( belt_temp(i,1) )) * belt_temp_scale + belt_temp_offset;
    temp2(i,1) = ( counts_to_volts ( temp2(i,1) )) * temp_scale + temp_offset;
    i_brake(i,1) = ( counts_to_volts ( i_brake(i,1) ) ) * i_brake_scale + i_brake_offset;
    temp1(i,1) = ( counts_to_volts ( temp1(i,1) )) * temp_scale + temp_offset;
    load_cell(i,1) = ( counts_to_volts ( load_cell(i,1) ) * load_cell_scale + load_cell_offset );
    tps(i,1) = ( counts_to_volts ( tps(i,1) ) * tps_scale + tps_offset;
end



