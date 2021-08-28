%% Initialization
clear; close all; clc

f = figure;
t = tiledlayout('flow','TileSpacing','compact');
gens = {'Table','PwCLT'};
for gg=1:numel(gens)
%     A = csvread(strcat('results/vld-results/runtimes_linux_',lower(gens{gg}),'.csv'));
    A = csvread(strcat('results/vld-results/runtimes_linux_',lower(gens{gg}),'.csv'));

    % Runtime order:
    % Image d4, Image d8, Image d16, Image d32, Image d64, spmv, randa,
    % rtb(insertion), rtb(lookup), strm (add), strm (copy), strm
    % (scale), strm (add), strm (triad), xsb
    
    % Linux  baseline FDU runs (master)
    fdu_106_85_stock = [0.304060; 0.129311; 0.065314; 0.018200; 0.005131;...
                        7.395966;  0.682189; 8.758910; 0.330395; 0.052419;...
                        0.054690; 0.106550; 0.108718; 81.061];
    fdu_400_200_stock = [0.589763; 0.280872; 0.141871; 0.039818; 0.011081;...
                        15.549092; 1.441151; 15.406092; 0.685404; 0.131510;... 
                        0.134760; 0.226782; 0.230861; 162.674];
                    
    % Linux  baseline FDU runs (187.5MHz) % These turned out to be wrong
    % when I checked the histograms of the 187.5MHz FDU branch.
%     fdu_106_85_stock = [0.318640; 0.141480; 0.071837; 0.019944; 0.005640; ...
%                         8.108245; 0.746114; 9.321568; 0.361769; 0.057743; ...
%                         0.060840; 0.116741;  0.118369; 88.250];
%     fdu_400_200_stock = [0.797792; 0.383151; 0.196278; 0.053856; 0.015090; ...
%                          21.148864; 1.978381; 19.957321; 0.928822; 0.212397; ...
%                          0.204769; 0.310968; 0.311729; 218.346];

    % Standalone baseline FDU runs -- comparing against these we realized
    % randa and rtb are very different although image&strm were fine
%     fdu_106_85_stock = [0.286834; 0.125916; 0.062097; 0.016402; 0.005106;...
%                         0.437967; 0.441957; 5.059381; 0.195671;...
%                         0.051172; 0.052914; 0.105815; 0.107458; 76.259];
%     fdu_400_200_stock = [0.544779; 0.264122; 0.132263; 0.035032; 0.010736;...
%                          0.876994; 0.924307; 8.795193; 0.402458;...
%                          0.12716; 0.127172; 0.220953; 0.22266; 152.668];

    % These are hardcoded, changing them is not sufficient for different
    % parsing pattern
    num_latencies = 2;
    num_divs = 4;
    result_cnts = [5 1 1 2 4];
    overall_cpu = zeros(num_latencies*num_divs, numel(result_cnts));

    %% Parsing

    % Image
    for i=1:8
        if i<=4
            overall_cpu(i,1) = geomean( abs(A((i-1)*5+1:(i-1)*5+5)-fdu_106_85_stock(1:5))./fdu_106_85_stock(1:5));
        else
            overall_cpu(i,1) = geomean( abs(A((i-1)*5+1:(i-1)*5+5)-fdu_400_200_stock(1:5))./fdu_400_200_stock(1:5));
        end
    end

    % SpMV, Randa 
    k=41;
    for i=6:7
        for n=1:8
            if n <=4
                overall_cpu(n,i-4) = (A(k)-fdu_106_85_stock(i))./fdu_106_85_stock(i);
            else
                overall_cpu(n,i-4) = (A(k)-fdu_400_200_stock(i))./fdu_400_200_stock(i);
            end
            k = k+1;
        end
    end

    % Rtb
    s=56;
    for k=1:8
        if k<=4
            overall_cpu(k,4) = geomean( abs(A((s+(k-1)*2+1):(s+(k-1)*2+2))-fdu_106_85_stock(8:9))./fdu_106_85_stock(8:9));
        else
            overall_cpu(k,4) = geomean( abs(A((s+(k-1)*2+1):(s+(k-1)*2+2))-fdu_400_200_stock(8:9))./fdu_400_200_stock(8:9));
        end
    end


    % STRM
    for t=1:8
        if t<=4
            overall_cpu(t,5) = geomean( abs(A(72+t:8:96+t)-fdu_106_85_stock(10:13))./fdu_106_85_stock(10:13));
        else
            overall_cpu(t,5) = geomean( abs(A(72+t:8:96+t)-fdu_400_200_stock(10:13))./fdu_400_200_stock(10:13));
        end
    end

    %% Plot the parsed arrays
    nexttile;
    
    leg_cpu = categorical({'image', 'spmv', 'randa', 'rtb', 'strm'});
    
    b=bar(leg_cpu, (100.*[overall_cpu]),1,'FaceColor','flat');
    for k = 1:size(b,2)/2
        b(k).CData = [0.05 0.2+k*0.2 0.05];
    end
    for k = size(b,2)/2+1:size(b,2)
        b(k).CData = [0.2 0.2 0.2+(k-4)*0.2];
    end
    title(strcat({'CPU '}, gens{gg}, '-based Benchmarks'))
    ylabel('Runtime Change (%)');
    xlabel('Applications');

end

lgd=legend({'\sigma=\mu/4','\sigma=\mu/8','\sigma=\mu/16','\sigma=\mu/32',...
    '\sigma=\mu/4','\sigma=\mu/8','\sigma=\mu/16','\sigma=\mu/32'},...
    'Location','northeast','NumColumns',2);
title(lgd,'t_{W/R}=106/85ns  t_{W/R}=400/200ns')     
%     lgd.Layout.Tile = 'east';
saveas(gcf,'figure_matlab.png')

