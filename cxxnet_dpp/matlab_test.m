load example/MNIST/wmat;
load example/MNIST/Louter;
load example/MNIST/Lsq;
load example/MNIST/Lm;

%%
A = wmat'*wmat;
max(max(abs(A - Louter)))

%%
max(max(abs(Louter.^2 - Lsq)))
%%
max(max(abs(Lm - Lsq)))

%%
load example/MNIST/wmat;
load example/MNIST/Y;
load example/MNIST/tmpw;
load example/MNIST/mask;
%%
close all;
plot(wmat(:, 1) - tmpw(:, 1)); hold on;
plot(wmat(:, 1), 'g');
plot(tmpw(:, 1), 'r');

%%
load example/MNIST/wmat;
load example/MNIST/wmat2;

%%
load example/MNIST/Y2;