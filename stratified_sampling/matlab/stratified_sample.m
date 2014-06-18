function sidx = stratified_sample(k, idx)
% Sample K total points from each of the clusters represented in IDX.
% The samples are returned in SIDX

clusters = unique(idx);
N = numel(clusters);

if mod(k, N) ~= 0
    error('stratified_sample.m error: k\%N ~= 0\n');
end

sidx = zeros(k, 1);
nCluster = k/N;

for i=1:N
    c_idx = find(idx==i);
    sidx((i-1)*nCluster+1:i*nCluster) = randsample(c_idx, nCluster); 
end

sidx = sort(sidx);