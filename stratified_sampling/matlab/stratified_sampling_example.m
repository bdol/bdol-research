% Generate the grid of points
n = 60;
[x y] = meshgrid((1:n)/n, (1:n)/n);
X = [x(:) y(:)];

% Cluster
kCluster = 100;
idx = kmeans(X, kCluster);

% Stratified sample
k = 100;
sidx = stratified_sample(k, idx);

% Plot
close all;
plot(X(sidx, 1), X(sidx, 2), 'bo');
title(sprintf('Stratified sample w/ kmeans. %d clusters, %d samples', ...
            kCluster, k));
                
%% Compare StratSamp (CPP version) to DPP and random independent
addpath(genpath('~/Dropbox/Code/dpp/'));
clear;
n = 60;
sigma = 0.1; % kernel width
[x y] = meshgrid((1:n)/n, (1:n)/n);
X = [x(:) y(:)];
numSamples = 100;

L = exp(- (bsxfun(@minus,x(:),x(:)').^2 + ...
           bsxfun(@minus,y(:),y(:)').^2) / sigma^2);

% sample
ind_sample = randsample(n*n,numSamples);
dpp_sample = sample_dpp(decompose_kernel(L), numSamples);
load ../cpp/samples.txt
strat_sample = samples+1;

% Plot
close all;
hold on;
subplot(1, 3, 1);
plot(X(ind_sample, 1), X(ind_sample, 2), 'ro');
title('Independent');
subplot(1, 3, 2);
plot(X(dpp_sample, 1), X(dpp_sample, 2), 'ko');
title('DPP');
subplot(1, 3, 3);
plot(X(strat_sample, 1), X(strat_sample, 2), 'bo');
title('Stratified Sampling');