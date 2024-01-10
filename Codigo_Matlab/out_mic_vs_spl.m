% Define the SPL and Vo ranges
SPL = 0:1:85;  % SPL values from 40 to 120 with a step of 1

% Calculate Vo for each SPL value
for i = 1:length(SPL)
    V(i) = 6.31*10^-3 * 10^(SPL(i)/20) * 20*10^-6;
end

% Plot the function
figure(1);
plot(SPL, V, 'b.-');
title('Vo vs SPL');
xlabel('SPL (dB)');
ylabel('Vo (V)');
grid on;
xticks(0:1:85)
