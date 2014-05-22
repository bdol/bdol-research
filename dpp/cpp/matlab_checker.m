load L.txt; load V.txt; load d.txt;
%%
n = 20;
sigma = 0.1; % kernel width

% choose a grid of points
[x y] = meshgrid((1:n)/n);

% gaussian kernel
Lm = exp(- (bsxfun(@minus,x(:),x(:)').^2 + ...
           bsxfun(@minus,y(:),y(:)').^2) / sigma^2);
[Vm, Dm] = eig(Lm);

%%
identDiff = max(max(abs(eye(size(V, 1)) - V*V')));
constructDiff = max(max(abs(L - V*diag(d)*V')));

mlVDiff = max(max(abs(abs(Vm) - abs(V))));
mldDiff = max(max(abs(d-diag(Dm))));

fprintf('\n\n');
fprintf('Ident diff: %e\n', identDiff);
fprintf('Construct diff: %e\n', constructDiff);
fprintf('V diff: %e\n', mlVDiff);
fprintf('d diff: %e\n', mldDiff);

%%
clear;
load V_before_col_del.txt;
load Vj.txt;
load V_col_del.txt;

%%
clear;
load Vi_before_scale.txt;
load Vi_after_scale.txt;
load V_col_del.txt;
load Vj.txt;

all(V_col_del(43, :) == Vi_before_scale)
all(Vi_after_scale == Vi_before_scale./Vj(43))

%%
clear;
load Vshift.txt;
load Vi_after_scale.txt;
load Vj.txt

all(all(Vshift == Vj*Vi_after_scale))

%%
clear;
load Vshift.txt
load V_col_del.txt
load V_after_minus.txt

all(all(V_after_minus == V_col_del-Vshift))

%%
clear;
load V_after_minus.txt
load V_after_qr.txt

[Q, R] = qr(V_after_minus, 0);
all(all(V_after_qr==Q))