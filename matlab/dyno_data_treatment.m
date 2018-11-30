filename = input('Enter file name: \n');
dir = input('Enter results directory: \n');
mkdir (fullfile(dir));

%scales, offsets
%update this one
torque_scale = 1;
torque_offset = 0;

temp_scale = 44.5;
temp_offset = 14.3;

belt_temp_scale = 445.8;
belt_temp_offset = -307.4;

i_brake_scale = 1;
i_brake_offset = 0.05;

load_cell_scale = 30.3;
load_cell_offset = -50;

%usused
tps_scale = 1;
tps_offset = 0;

%parse data from files
%each quantity has its own column
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
    tps(i,1) = ( counts_to_volts ( tps(i,1) ) * tps_scale + tps_offset );
end

%calculate data
engine_power = torque .* prim_rpm / 5252;
wheel_power = load_cell .* sec_rpm / 5252;
powertrain_efficiency = wheel_power ./ engine_power;

%plot data
x = [1:num_rows];
scrsz = get(0,'ScreenSize');

%RPM
f = figure('Position',[1 scrsz(4)/2-80 scrsz(3)/2 scrsz(4)/2]);
p = plot(x,prim_rpm,x,sec_rpm);
p(1).Color = [1,0,0];
p(2).Color = [0,1,0];
title('RPM')
xlabel('time')
ylabel('RPM')
legend('Primary','Secondary')
fullFileName = fullfile(dir,'RPM.jpg');
saveas(f,fullFileName);

%Temperatures
f = figure('Position',[1 1 scrsz(3)/2 scrsz(4)/2]);
p = plot(x,temp1,x,temp2,x,temp3,x,belt_temp);
p(1).Color = [1,0,0];
p(2).Color = [0,1,0];
p(3).Color = [0,0,1];
p(4).Color = [0,1,1];
title('Temperatures')
xlabel('time')
ylabel('Temperature')
legend('Temp1','Temp2','Temp3','Belt Temp')
fullFileName = fullfile(dir,'temps.jpg');
saveas(f,fullFileName);

%Torques
f = figure('Position',[scrsz(3)/2 2*scrsz(4)/3-80 scrsz(3)/2 scrsz(4)/3]);
p = plot(x,torque,x,load_cell);
p(1).Color = [1,0,0];
p(2).Color = [0,1,0];
title('Torque')
xlabel('time')
ylabel('Torque')
legend('Primary','Secondary')
fullFileName = fullfile(dir,'torque.jpg');
saveas(f,fullFileName);

%Brake current
f = figure('Position',[scrsz(3)/2 scrsz(4)/3 scrsz(3)/2 scrsz(4)/3]);
p = plot(x,i_brake);
title('Brake Current')
xlabel('time')
ylabel('Current')
fullFileName = fullfile(dir,'brake current.jpg');
saveas(f,fullFileName);

%Setpoints
f = figure('Position',[scrsz(3)/2 1 scrsz(3)/2 scrsz(4)/3]);
ax1 = subplot(2,1,1);
plot(ax1,x,i_sp);
title(ax1,'Current setpoint')
xlabel(ax1,'time')
ylabel(ax1,'Set point (0-100%)')

ax2 = subplot(2,1,2);
plot(ax2,x,tps_sp);
title(ax2,'Throttle setpoint')
xlabel(ax2,'time')
ylabel(ax2,'Set point (0-100%)')
fullFileName = fullfile(dir,'setpoints.jpg');
saveas(f,fullFileName);

%Power
f = figure('Position',[scrsz(3)/2 1 scrsz(3)/2 scrsz(4)/3]);
ax1 = subplot(2,1,1);
plot(ax1,x,engine_power);
title(ax1,'Engine power')
xlabel(ax1,'time')
ylabel(ax1,'HP')

ax2 = subplot(2,1,2);
plot(ax2,x,wheel_power);
title(ax2,'Output Power')
xlabel(ax2,'time')
ylabel(ax2,'HP')
fullFileName = fullfile(dir,'power.jpg');
saveas(f,fullFileName);

%efficiency
f = figure('Position',[scrsz(3)/2 scrsz(4)/3 scrsz(3)/2 scrsz(4)/3]);
p = plot(x,powertrain_efficiency);
title('Powertrain Effeciency')
xlabel('time')
ylabel('%')
fullFileName = fullfile(dir,'efficiency.jpg');
saveas(f,fullFileName);

fullFileName = fullfile(dir,'data.mat');
save(fullFileName);
