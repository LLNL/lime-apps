clear; close all; clc

datapath = "C:\Users\mhusr\Desktop\LLNL2\VDU\4ccad1\trace\tools\";
filename = "table106W85R32_l.dat"; % Produced by first processing the raw trace
    % using lime/parser, then lime/tools/lplot.sh

experiments = {'image','randa','strm','xsb'};

f = figure;
t = tiledlayout('flow','TileSpacing','compact');

for i=1:numel(experiments)
    nexttile;
    table106W85R32l = table2array(readtable(strcat(datapath,experiments{i},filename))); % This is LIME trace
    latencychecker = readmatrix(strcat("../",experiments{i},".csv"))+5.33; % This is what Ivy provided
    stairs(table106W85R32l(:,2)./sum(table106W85R32l(:,2))); hold on;
    stairs(table106W85R32l(:,3)./(sum(table106W85R32l(:,3)))); hold on
    title(experiments{i})
    h=histogram(latencychecker,numel(table106W85R32l(:,2)),'Normalization','probability', 'DisplayStyle', 'stairs');
    legend('LiME VLD Bare Metal CPU Read','LiME VLD Bare Metal CPU Write','Measurement on real hardware')
    xlabel('Latency (ns)')
    ylabel('Probability')
    nss = mean(latencychecker) % This is the mean of Ivy's results in nanoseconds, to be programmed to FDU for comparison
    ccs = round(  mean(latencychecker) * (20 * 300) / (187.5 * 5.33) ) % The same in clock cycles (to be later provided to
        % ivy_custom_delay.py by prep_linux_variation_experiments.sh)
end
 
results_fdu_linux = [0.019850 0.399823 0.106657 0.108101 0.212245 0.216198]; % image75W75R randa41W41R strm186W186R
results_gd4_table_linux = [0.022407 0.416791 0.120576 0.121065 0.233229 0.234780];
results_gd8_table_linux = [0.020859 0.416090 0.115605 0.115698 0.223544 0.224756];
results_gd16_table_linux = [0.020279 0.417248 0.112825 0.113068 0.218082 0.218695];
results_gd32_table_linux = [0.020154 0.416443 0.109885 0.110983 0.216136 0.216813];
results_ivy_table_linux = [0.022492 0.379435 0.109473 0.111103 0.212957 0.213824];

results_linux = [results_fdu_linux; results_gd4_table_linux; results_gd8_table_linux; results_gd16_table_linux;...
                 results_gd32_table_linux; results_ivy_table_linux];

f2 = figure;
leg_cpu = categorical({'image', 'randa', 'strm-copy', 'strm-scale', 'strm-add', 'strm-triad'});

b=bar(leg_cpu, transpose(results_linux),1,'FaceColor','flat');
for k = 2:5
    b(k).CData = [0.05 0.2+(k-1)*0.2 0.05];
end
b(1).CData=[0 0 1];
b(6).CData=[0.6350 0.0780 0.1840];
ylabel('Runtime (s)');
xlabel('Applications');
legend('FDU @ measurement mean','VDU DT (\sigma=\mu/4 Gaussian @ measurement mean)',...
    'VDU DT (\sigma=\mu/8 Gaussian @ measurement mean)', 'VDU DT (\sigma=\mu/16 Gaussian @ measurement mean)',...
    'VDU DT (\sigma=\mu/32 Gaussian @ measurement mean)', 'VDU DT (Populated with real measurements)');


results_linux_relative = ([results_fdu_linux; results_gd4_table_linux; results_gd8_table_linux; results_gd16_table_linux;...
                          results_gd32_table_linux]-results_ivy_table_linux)./results_ivy_table_linux;

f3 = figure;
leg_cpu = categorical({'image', 'randa', 'strm-copy', 'strm-scale', 'strm-add', 'strm-triad'});
b=bar(leg_cpu, 100.*transpose(results_linux_relative),1,'FaceColor','flat');
for k = 2:5
    b(k).CData = [0.05 0.2+(k-1)*0.2 0.05];
end
b(1).CData=[0 0 1];
ylabel('Runtime difference vs. real measurement (%)');
xlabel('Applications');
legend('FDU @ measurement mean','VDU DT (\sigma=\mu/4 Gaussian @ measurement mean)',...
    'VDU DT (\sigma=\mu/8 Gaussian @ measurement mean)', 'VDU DT (\sigma=\mu/16 Gaussian @ measurement mean)',...
    'VDU DT (\sigma=\mu/32 Gaussian @ measurement mean)');



