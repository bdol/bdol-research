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