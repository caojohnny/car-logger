clc; clf; clear;

data = readtable("data/collected_data.csv");

time_s = data.time_ms / 1000;
window = (time_s > 100) & (time_s < 335);
time_s = time_s(window);

tach = data.engineSpeed_rpm(window);
speed = data.vehicleSpeed_kph(window);
throttle = data.throttlePosition_pct(window);

figure(1);
subplot(3, 1, 1);
plot(time_s, tach);
title("Tachometer");
xlabel("Time (s)");
ylabel("Engine RPM");
grid();

subplot(3, 1, 2);
plot(time_s, speed);
title("Speedometer");
xlabel("Time (s)");
ylabel("Vehicle Speed (kph)");
grid();

subplot(3, 1, 3);
plot(time_s, throttle);
title("Throttle");
xlabel("Time (s)");
ylabel("Throttle Position (%)");
grid();
