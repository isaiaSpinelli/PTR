function [A,F] = calcul_fft(u, nbr_pt, fe)

%Calcul de la FFT
Y_complexe=fft(u,nbr_pt);
Y_module = abs(Y_complexe/nbr_pt); %Calcul du module
Y_mod_2 = 2*Y_module(1:nbr_pt/2); %Selection des resultat jusqu'� la frequence de Nyquist

f1 = (0:(nbr_pt/2)-1)*fe/nbr_pt; %frequence sur l'axe des abscisses

%Valeur de retour
A=Y_mod_2;
F=f1;

end
