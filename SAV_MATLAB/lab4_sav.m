% Labo4
clear all; close all;

info = audiodevinfo()

recObj = audiorecorder(48000,16,1);

disp('Start speaking.')
recordblocking(recObj, 2);
disp('End of Recording.');

play(recObj);
y = getaudiodata(recObj);

figure
plot(y);

Ymilieu = y(30001:1:78000);

% Calculs de FFT
[A, F] = calcul_fft(Ymilieu, 48000, 8000);

figure
plot(F,A)

