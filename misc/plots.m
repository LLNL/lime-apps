%% Initialization
clear; close all; clc

% Load Octave related packages
if isunix()
    pkg load statistics
    graphics_toolkit gnuplot
end

% This MATLAB/Octave script reads the following CSV file, parses the run 
% times and plots the variation bar charts. It is tested in MATLAB R2021a
% and Octave 5.2 on Cygwin.
if isunix()
    A = csvread('runtimes_final.csv');
else
    A = csvread('results/vld-results/runtimes_pwclt.csv');
%     A = csvread('../runtimes_interconnect21_final_backup.csv');
end

% Baseline FDU runs
% Image d4, Image d8, Image d16, Image d32, Image d64, pager, spmv, randa,
% rtb(first number), rtb(second number), strm (add), strm (copy), strm
% (scale), strm (add), strm (triad), xsb
fdu_106_85_stock = [0.15344; 0.125916; 0.062097; 0.016402; 0.005106;...
                    0.284904; 0.437967; 0.441957; 5.059381; 0.195671;...
                    0.051172; 0.052914; 0.105815; 0.107458; 76.259];
fdu_400_200_stock = [0.544779; 0.264122; 0.132263; 0.035032; 0.010736;...
                     0.541041; 0.876994; 0.924307; 8.795193; 0.402458;...
                     0.12716; 0.127172; 0.220953; 0.22266; 152.668];
                 
% Image d4, Image d8, Image d16, Image d32, Image d64, pager, spmv, randa,
% rtb(first number), rtb(second number)
fdu_106_85_cloff = [0.153379; 0.038616; 0.009821; 0.002582; 0.000741;...
                    0.2839; 0.294397; 0.133285; 3.731161; 0.037343];
fdu_400_200_cloff = [0.257754; 0.064799; 0.016451; 0.004297; 0.001233;...
                     0.559104; 0.603417; 0.2143; 6.45142; 0.054611]; 

% These are hardcoded, changing them is not sufficient for different
% persing pattern
num_latencies = 2;
num_divs = 4;
result_cnts = [5 1 1 1 2 4 1];
acc_enabled = [1 1 1 1 1 0 0];
overall_cpu = zeros(num_latencies*num_divs, numel(result_cnts));
overall_acc = zeros(num_latencies*num_divs, sum(acc_enabled));

%% Parsing

% Image
for i=1:8
    if i<=4
        overall_cpu(floor((2*i-1)/2)+1,1) = geomean( abs(A((i-1)*10+1:(i-1)*10+5)-fdu_106_85_stock(1:5))./fdu_106_85_stock(1:5));
        overall_acc(floor((2*i-1)/2)+1,1) = geomean( abs(A((i-1)*10+6:(i-1)*10+10)-fdu_106_85_cloff(1:5))./fdu_106_85_cloff(1:5));
    else
        overall_cpu(floor((2*i-1)/2)+1,1) = geomean( abs(A((i-1)*10+1:(i-1)*10+5)-fdu_400_200_stock(1:5))./fdu_400_200_stock(1:5));
        overall_acc(floor((2*i-1)/2)+1,1) = geomean( abs(A((i-1)*10+6:(i-1)*10+10)-fdu_400_200_cloff(1:5))./fdu_400_200_cloff(1:5));
    end
end

% PageRank, SpMV, Randa 
k=81;
for i=6:8
    for n=1:16
        if n <=8
            if mod(k,2)==1
                overall_cpu(floor((n-1)/2+1),i-4) = abs(A(k)-fdu_106_85_stock(i))./fdu_106_85_stock(i);
            else
                overall_acc(floor((n-1)/2+1),i-4) = abs(A(k)-fdu_106_85_cloff(i))./fdu_106_85_cloff(i);
            end
        else
            if mod(k,2)==1
                overall_cpu(floor((n-1)/2+1),i-4) = abs(A(k)-fdu_400_200_stock(i))./fdu_400_200_stock(i);
            else
                overall_acc(floor((n-1)/2+1),i-4) = abs(A(k)-fdu_400_200_cloff(i))./fdu_400_200_cloff(i);
            end
        end
        k = k+1;
    end
end

% Rtb
s=128;
for k=1:8
    if k<=4
        overall_cpu(floor((2*k-1)/2)+1,5) = geomean( abs(A((s+(k-1)*4+1):(s+(k-1)*4+2))-fdu_106_85_stock(9:10))./fdu_106_85_stock(9:10));
        overall_acc(floor((2*k-1)/2)+1,5) = geomean( abs(A((s+(k-1)*4+3):(s+(k-1)*4+4))-fdu_106_85_cloff(9:10))./fdu_106_85_cloff(9:10));
    else
        overall_cpu(floor((2*k-1)/2)+1,5) = geomean( abs(A((s+(k-1)*4+1):(s+(k-1)*4+2))-fdu_400_200_stock(9:10))./fdu_400_200_stock(9:10));
        overall_acc(floor((2*k-1)/2)+1,5) = geomean( abs(A((s+(k-1)*4+3):(s+(k-1)*4+4))-fdu_400_200_cloff(9:10))./fdu_400_200_cloff(9:10));
    end
end


% STRM
for t=1:8
    if t<=4
        overall_cpu(t,6) = geomean( abs(A(160+t:8:184+t)-fdu_106_85_stock(11:14))./fdu_106_85_stock(11:14));
    else
        overall_cpu(t,6) = geomean( abs(A(160+t:8:184+t)-fdu_400_200_stock(11:14))./fdu_400_200_stock(11:14));
    end
end

% XSBench
overall_cpu(1:4,7) = abs(A(193:196)-fdu_106_85_stock(15))./fdu_106_85_stock(15);
overall_cpu(5:8,7) = abs(A(197:200)-fdu_400_200_stock(15))./fdu_400_200_stock(15);

%% Plot the parsed arrays

if isunix()
    f = figure;
    set(f, "visible", "off")

    leg_cpu = {'image', 'pager', 'spmv', 'randa', 'rtb', 'strm', 'xsb'};
    leg_acc = {'image', 'pager', 'spmv', 'randa', 'rtb'};
    subplot(1,2,1)
    b=bar(1:7, transpose(100.*overall_cpu),1,'FaceColor','flat');
    for k = 1:size(b,2)/2
        set ( b(k), 'facecolor', [0.05 0.2+k*0.2 0.05])
    end
    for k = size(b,2)/2+1:size(b,2)
        set ( b(k), 'facecolor', [0.05 0.05 0.1+(k-4)*0.22])
    end
    set(gca, 'XTickLabel', leg_cpu);  
    ylabel('Runtime Change (%)');
    xlabel('CPU');

    subplot(1,2,2)
    b2=bar(1:5,transpose(100.*overall_acc),1,'FaceColor','flat');
    for k = 1:size(b2,2)/2
        set ( b2(k), 'facecolor', [0.05 0.2+k*0.2 0.05])
    end
    for k = size(b2,2)/2+1:size(b2,2)
        set ( b2(k), 'facecolor', [0.05 0.05 0.1+(k-4)*0.22])
    end
    
    set(gca, 'XTickLabel', leg_acc);
    ylabel('Runtime Change (%)');
    xlabel('Accelerator');

    lgd=legend('t_{W/R}=106/85ns \sigma=\mu/4','t_{W/R}=106/85ns \sigma=\mu/8',...
    't_{W/R}=106/85ns \sigma=\mu/16','t_{W/R}=106/85ns \sigma=\mu/32',...
    't_{W/R}=400/200ns \sigma=\mu/4','t_{W/R}=400/200ns \sigma=\mu/8',...
    't_{W/R}=400/200ns \sigma=\mu/16','t_{W/R}=400/200ns \sigma=\mu/32','location','bestoutside');
    print (f, "figure_octave.png",'-dpng', '-S840,360');
else
    f = figure;
    t = tiledlayout('flow','TileSpacing','compact');
    nexttile;
    
    leg_cpu = categorical({'image', 'pager', 'spmv', 'randa', 'rtb', 'strm', 'xsb'});
    leg_acc = categorical({'image', 'pager', 'spmv', 'randa', 'rtb'});
    
    b=bar(leg_cpu, (100.*overall_cpu),1,'FaceColor','flat');
    for k = 1:size(b,2)/2
        b(k).CData = [0.05 0.2+k*0.2 0.05];
    end
    for k = size(b,2)/2+1:size(b,2)
        b(k).CData = [0.2 0.2 0.2+(k-4)*0.2];
    end
    ylabel('Runtime Change (%)');
    xlabel('CPU');

    nexttile;
    b2=bar(leg_acc,100.*overall_acc,1,'FaceColor','flat');
    for k = 1:size(b2,2)/2
        b2(k).CData = [0.05 0.2+k*0.2 0.05];
    end
    for k = size(b2,2)/2+1:size(b2,2)
        b2(k).CData = [0.05 0.05 0.2+(k-4)*0.2];
    end
    ylabel('Runtime Change (%)');
    xlabel('Accelerator');
    lgd=legend({'\sigma=\mu/4','\sigma=\mu/8','\sigma=\mu/16','\sigma=\mu/32',...
        '\sigma=\mu/4','\sigma=\mu/8','\sigma=\mu/16','\sigma=\mu/32'},...
        'Location','northeast','NumColumns',2);
    title(lgd,'t_{W/R}=106/85ns  t_{W/R}=400/200ns')     
    lgd.Layout.Tile = 'east';
    saveas(gcf,'figure_matlab.png')
end
